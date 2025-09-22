#include "InGameSystem.h"

#include "Events.h"
#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"
#include "Player.h"
#include "StageEditorSystem.h"
#include "CharaEditorSystem.h"
#include "StageRegistry.h"
#include "CharaRegistry.h"
#include "Cursed/BalatroSystem.h"
#include "Settings.h"

#include <Core/Time.h>
#include <Core/Window.h>
#include <Core/JobManager.h>
#include <Core/VarRegistry.h>
#include <Util/Globals.h>
#include <Util/StrUtil.h>
#include <Event/EventBus.h>
#include <Input/Input.h>
#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>
#include <Renderer/GraphicsCommandBuffer.h>
#include <Thirdparty/imgui/imgui.h>
#include <json/json.hpp>

#include "MainMenuSystem.h"

Funkin::GameState gGameState;

#undef min
#undef max

float gMissTimer = 0.0f;
constexpr static float C_COUNTDOWN_DUR = 1.5f;

Stratum::ECS::edict_t MainCameraEntity = 0;
Stratum::ECS::edict_t GuiCameraEntity = 0;

Funkin::InGameSystem::InGameSystem(const LoadChartParams& params) : mLoadParams(params)
{
	mSong = params.SongScript;
	mLoadingDone.store(false);
	Settings::s_Settings->LoadFromFile("settings.json");
}

Funkin::InGameSystem::~InGameSystem()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
	pauseSource->Stop();
	Settings::s_Settings->SaveToFile("settings.json");
}

void Funkin::InGameSystem::Init(Stratum::Scene* scene)
{
	gGameState = {};
	mScene = scene;

	this->mCountdownTimer = C_COUNTDOWN_DUR;

	StageRegistry::Init(mScene);
	CharaRegistry::Init(mScene);

	{
		auto entity = MainCameraEntity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
	}

	{
		auto entity = GuiCameraEntity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
		camera.RenderLayer = 4;
	}

	{
		auto entity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
		camera.RenderLayer = 5;
	}

	scrollSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/sounds/scrollMenu.mp3", mScene->AudioEngine->GetEngine());
	pauseSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/music/breakfast.mp3", mScene->AudioEngine->GetEngine());
	scene->AudioEngine->AddSource(scrollSource);
	scene->AudioEngine->AddSource(pauseSource);
	
	pauseSource->SetLooping(true);

	scrollSource->SetVolume(0.25f);
	pauseSource->SetVolume(0.35f);

	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteHoldComponent>(), C_NOTE_HOLD_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<AnimatedEffectComponent>(), C_ANIMATED_EFFECT_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<StagePropComponent>(), C_STAGE_PROP_COMPONENT_NAME);

	pTimedActionSystem = new TimedActionSystem();
	mConductor = new Conductor();
	gGameState.pConductor = mConductor;
	gGameState.pInGame = this;

	PlayerSystem* playerSystem;

	mScene->RegisterCustomSystem(mConductor, true);
	mScene->RegisterCustomSystem(pTimedActionSystem, true);
	mScene->RegisterCustomSystem(playerSystem = new PlayerSystem(mConductor, &gGameState), true);

	gGameState.pPlayerSystem = playerSystem;

	mConductor->LoadChart(mScene, mLoadParams.ChartPath);

	mLoadingStage.fetch_add(1);

	if (!mLoadParams.OverrideStage.empty())
		mConductor->chart.info.stage = mLoadParams.OverrideStage;

	if (!mLoadParams.OverridePlayer1.empty())
		mConductor->chart.info.player1 = mLoadParams.OverridePlayer1;

	StageRegistry::AddStage(mConductor->chart.info.stage);

	mLoadingStage.fetch_add(1);

	CharaRegistry::AddCharacter(mConductor->chart.info.player1);

	mLoadingStage.fetch_add(1);

	CharaRegistry::AddCharacter(mConductor->chart.info.player2);

	mLoadingStage.fetch_add(1);

	mSong->Init(mConductor, this, mScene);

	StageRegistry::SetStage(mConductor->chart.info.stage);
	mLoadingStage.fetch_add(1);

	mScene->FontRegistry.LoadFont("Funkin", "fonts/Phantomuff Difficult Font.ttf");

	std::string instPath = C_SONG_PATH_PREFIX;
	std::string voicesPath = "fnf/songs/";
	instPath.append(mConductor->chart.info.song).append("/Inst.mp3");
	voicesPath.append(mConductor->chart.info.song).append("/Voices.mp3");

	this->SetPlayerCharacter(CharaRegistry::GetCharacter(mConductor->chart.info.player1));
	this->SetOpponentCharacter(CharaRegistry::GetCharacter(mConductor->chart.info.player2));

	for (int i = 0; i < 3; i++)
	{
		std::string path = "fnf/sounds/missnote";
		path.append(std::to_string(i + 1)).append(".mp3");
		missSources[i] = Stratum::CreateRef<Stratum::MP3AudioSource>(path.c_str(), scene->AudioEngine->GetEngine());
		missSources[i]->SetVolume(0.25f);
		scene->AudioEngine->AddSource(missSources[i]);
	}

	mLoadingStage.fetch_add(1);

	mConductor->RegisterEventHandler("StSetScreenBeat", [this](ChartEvent& event)
		{
			gGameState.DoBeatEveryNthBeat = event.castInteger(event.Arg1);

			int offset = event.castInteger(event.Arg2);

			if (offset != 0)
				gGameState.BeatOffset = offset;
		});
	mConductor->RegisterEventHandler("StFadeToWhite", [this](ChartEvent& event)
		{
			auto& whiteSprite = mScene->SpriteRenderers.Get(mWhiteSprite);
			whiteSprite.SpriteColor = glm::vec4(1.0f, 1.0f, 1.0f, whiteSprite.SpriteColor.a);

			mFadeToWhiteIntensity = event.castFloat(event.Arg1);
			mFadeToWhiteBaseTime = event.castInteger(event.Arg2) / 1000.0f;
			mFadeToWhiteTime = 0.0f;
		});
	mConductor->RegisterEventHandler("StFadeToBlack", [this](ChartEvent& event)
		{
			auto& whiteSprite = mScene->SpriteRenderers.Get(mWhiteSprite);
			whiteSprite.SpriteColor = glm::vec4(0.0f, 0.0f, 0.0f, whiteSprite.SpriteColor.a);

			mFadeToWhiteIntensity = event.castFloat(event.Arg1);
			mFadeToWhiteBaseTime = event.castInteger(event.Arg2) / 1000.0f;
			mFadeToWhiteTime = 0.0f;
		});
	mConductor->RegisterEventHandler("StSetStage", [this](ChartEvent& event)
		{
			StageRegistry::SetStage(event.Arg1);
		});
	mConductor->RegisterEventHandler("StSetPlayerCharacter", [this](ChartEvent& event)
		{
			this->SetPlayerCharacter(CharaRegistry::GetCharacter(event.Arg1));
		});

	mWhiteSprite = mScene->EntityManager.CreateEntity();

	mScene->Transforms.Create(mWhiteSprite);

	auto& sprite = mScene->SpriteRenderers.Create(mWhiteSprite);
	sprite.Rect = { glm::ivec2(0, 0), glm::ivec2(10000, 10000) };
	sprite.CameraLayer = 4;
	sprite.RenderLayer = 100;
	sprite.SpriteColor.a = 0.0f;

	instSource = Stratum::CreateRef<Stratum::MP3AudioSource>(instPath.c_str(), scene->AudioEngine->GetEngine());
	scene->AudioEngine->AddSource(instSource);

	if (mConductor->chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), scene->AudioEngine->GetEngine());
		scene->AudioEngine->AddSource(voicesSource);
	}

	mLoadingStage.fetch_add(1);

	mScene->Resources.LoadTextureImage("fnf/images/countdown/funkin/go.png");
	mScene->Resources.LoadTextureImage("fnf/images/countdown/funkin/set.png");
	mScene->Resources.LoadTextureImage("fnf/images/countdown/funkin/ready.png");

	pEarlyUpdate = true;

	mLoadingStage.fetch_add(1);

	Stratum::Time::EndProfile();
	Stratum::Time::BeginProfile();

	ma_engine_set_volume(mScene->AudioEngine->GetEngine(), 1.0f);
	ma_engine_set_gain_db(mScene->AudioEngine->GetEngine(), 2.0f);
	
	mLoadingDone.store(true);

	mResumeText = mScene->EntityManager.CreateEntity();
	mVolumeText = mScene->EntityManager.CreateEntity();
	mBotplayText = mScene->EntityManager.CreateEntity();
	mBalatroText = mScene->EntityManager.CreateEntity();
	mExitText = mScene->EntityManager.CreateEntity();
	mSelectText = mScene->EntityManager.CreateEntity();

	// Create pause UI buttons
	{
		auto entity = mResumeText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 90.0f;
		scene->TextComponents.Get(entity).Text = L"resume";
		scene->TextComponents.Get(entity).Font = "Funkin";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += 70.0f;
		scene->GuiAnchors.Get(entity).Position.x += 200.0f;
	}

	{
		auto entity = mVolumeText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 90.0f;
		scene->TextComponents.Get(entity).Text = L"Volume";
		scene->TextComponents.Get(entity).Font = "Funkin";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += 0.0f;
		scene->GuiAnchors.Get(entity).Position.x += 200.0f;
	}

	{
		auto entity = mBotplayText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 90.0f;
		scene->TextComponents.Get(entity).Text = L"Botplay: ";
		scene->TextComponents.Get(entity).Font = "Funkin";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += -70.0f;
		scene->GuiAnchors.Get(entity).Position.x += 200.0f;
	}

	{
		auto entity = mBalatroText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 90.0f;
		scene->TextComponents.Get(entity).Text = L"Balatro?";
		scene->TextComponents.Get(entity).Font = "Funkin";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += -70.0f;
		scene->GuiAnchors.Get(entity).Position.x += 200.0f;
	}

	{
		auto entity = mExitText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 90.0f;
		scene->TextComponents.Get(entity).Text = L"exit to desktop";
		scene->TextComponents.Get(entity).Font = "Funkin";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += -70.0f;
		scene->GuiAnchors.Get(entity).Position.x += 200.0f;
	}

	{
		auto entity = mSelectText;
		scene->TextComponents.Create(entity);
		scene->TextComponents.Get(entity).FontSize = 64.0f;
		scene->TextComponents.Get(entity).Text = L">";
		scene->TextRenderers.Create(entity).Alignment = 0.0f;
		scene->TextRenderers.Get(entity).RenderLayer = 10000;
		scene->TextRenderers.Get(entity).CameraLayer = 5;
		scene->Transforms.Create(entity);
		scene->GuiAnchors.Create(entity).AnchorPoint = Stratum::GuiAnchorPoint::LEFT;
		scene->GuiAnchors.Get(entity).Position.y += -70.0f;
		scene->GuiAnchors.Get(entity).Position.x += 150.0f;
	}
}

void Funkin::InGameSystem::OnActivate(Stratum::Scene* scene)
{
	auto missListener = [this](const NoteEvent& e)
		{
			if (!e.IsMiss || e.IsOponent)
				return;
			if (voicesSource)
				voicesSource->SetVolume(0.2f);
			Stratum::Input::SetGamepadRumble(1.0f, 1.0f, 200);
			missSources[rand() % 3]->Play();
			gMissTimer = 0.4f;
		};
	auto hitListener = [this](const NoteEvent& e)
		{
			if (e.IsMiss || e.IsOponent)
				return;
			if (voicesSource)
				voicesSource->SetVolume(1.0f);
		};
	auto opponentListener = [this](const NoteEvent& e)
		{
			if (!mOponentCharacter || !e.IsOponent)
				return;

			const char* animations[4] =
			{
				"left",
				"down",
				"up",
				"right"
			};

			std::string anim = animations[e.NoteType];

			if (mConductor->GetNoteByIndex(e.SectionIndex, e.NoteIndex).noteData.compare("No Animation") == 0)
			{
				return;
			}

			if (mConductor->GetNoteByIndex(e.SectionIndex, e.NoteIndex).noteData.compare("Alt Animation") == 0)
			{
				anim.append("Alt");
			}

			mOponentCharacter->PlayAnimation(anim);
		};

	Stratum::EventBus::RegisterListener<NoteEvent>(missListener, Stratum::EF_REMOVE_ON_SCENE_LOAD);
	Stratum::EventBus::RegisterListener<NoteEvent>(hitListener, Stratum::EF_REMOVE_ON_SCENE_LOAD);
	Stratum::EventBus::RegisterListener<NoteEvent>(opponentListener, Stratum::EF_REMOVE_ON_SCENE_LOAD);
}

void Funkin::InGameSystem::Update(Stratum::Scene* scene)
{
	if (!mHasSongStarted && mCountdownTimer <= 0.0f)
	{
		mHasSongStarted = true;
		instSource->Play();
		if (voicesSource)
			voicesSource->Play();
		mConductor->SongStarted = true;
	}

	if (mCountdownTimer > 0.0f)
	{
		static bool playedThree = false;
		static bool playedTwo = false;
		static bool playedOne = false;
		static bool playedGo = false;

		if (mCountdownTimer == C_COUNTDOWN_DUR)
		{
			playedThree = false;
			playedTwo = false;
			playedOne = false;
			playedGo = false;
		}

		mCountdownTimer -= Stratum::gpGlobals->deltaTime;

		uint32_t dec = mCountdownTimer * 10.0f;
		const uint32_t C_DEC = C_COUNTDOWN_DUR * 10.0f;
		const uint32_t C_THREE = 11;
		const uint32_t C_TWO = 8;
		const uint32_t C_ONE = 5;
		const uint32_t C_GO = 2;

		if (dec == C_THREE && !playedThree)
		{
			mScene->AudioEngine->PlayOneShot("fnf/sounds/gameplay/countdown/funkin/introTHREE.mp3");
			playedThree = true;
		}
		if (dec == C_TWO && !playedTwo)
		{
			mScene->AudioEngine->PlayOneShot("fnf/sounds/gameplay/countdown/funkin/introTWO.mp3");
			playedTwo = true;
			auto entity = this->CreateSpriteEntity("fnf/images/countdown/funkin/ready.png", {}, { 1.0f, 1.0f }, 5, 10000);
			auto& sprite = mScene->SpriteRenderers.Get(entity);
			pTimedActionSystem->PushAction(&sprite.SpriteColor.a, 0.0f, 0.3f, Easing::Linear, [entity, this]
				{
					mScene->EntityManager.DestroyEntity(entity);
				});
		}
		if (dec == C_ONE && !playedOne)
		{
			mScene->AudioEngine->PlayOneShot("fnf/sounds/gameplay/countdown/funkin/introONE.mp3");
			playedOne = true;
			auto entity = this->CreateSpriteEntity("fnf/images/countdown/funkin/set.png", {}, { 1.0f, 1.0f }, 5, 10000);
			auto& sprite = mScene->SpriteRenderers.Get(entity);
			pTimedActionSystem->PushAction(&sprite.SpriteColor.a, 0.0f, 0.3f, Easing::Linear, [entity, this]
				{
					mScene->EntityManager.DestroyEntity(entity);
				});
		}
		if (dec == C_GO && !playedGo)
		{
			mScene->AudioEngine->PlayOneShot("fnf/sounds/gameplay/countdown/funkin/introGO.mp3");
			playedGo = true;
			auto entity = this->CreateSpriteEntity("fnf/images/countdown/funkin/go.png", {}, { 1.0f, 1.0f }, 5, 10000);
			auto& sprite = mScene->SpriteRenderers.Get(entity);
			pTimedActionSystem->PushAction(&sprite.SpriteColor.a, 0.0f, 0.3f, Easing::Linear, [entity, this]
				{
					mScene->EntityManager.DestroyEntity(entity);
				});
		}

	}

	if (mConductor->BeatCountF < 3.0f)
	{
		if (voicesSource)
			voicesSource->SetVolume(1.0f);
		instSource->SetVolume(1.0f);

	}

	if (Stratum::Input::GetKeyDown(KeyCode::F11) || Stratum::Input::GetGamepadButtonDown(GamepadButton::BACK))
	{
		static bool fs = false;
		scene->Window->SetFullScreen(fs = !fs);
	}

	if (Stratum::Input::GetKeyDown(KeyCode::NUMBER_7))
	{
		auto editor = new Stratum::Scene();

		editor->RegisterCustomSystem(new StageEditorSystem(mLoadParams.ChartPath));

		scene->SwapScene(editor);
	}

	if (Stratum::Input::GetKeyDown(KeyCode::NUMBER_8))
	{
		auto editor = new Stratum::Scene();

		editor->RegisterCustomSystem(new CharaEditorSystem(mLoadParams.ChartPath));

		scene->SwapScene(editor);
	}

	mSong->Update();

	if (gMissTimer > 0.0f)
	{
		gMissTimer -= Stratum::gpGlobals->deltaTime;
		if (gMissTimer <= 0.0f)
		{
			if (voicesSource)
			{
				voicesSource->SetVolume(1.0f);
			}
		}
	}

	float bpmPerSecond = 1.0f / (mConductor->chart.info.bpm / 60.0f);
	static float songDeltaTime = 0.0f;
	static float lastSongTime = 0.0f;
	static glm::vec3 bfPosition = glm::vec3(0.0f);

	static float GuiZoomLevel = 1.0f;
	static float ZoomLevel = 1.0f;
	static bool DoBeat = false;

	if (instSource->IsPlaying())
	{
		mConductor->SongTime += Stratum::gpGlobals->deltaTime;
		// Correct SongTime based on the instSource actual position
		// Keeps smooth feeling on frame based time tracking without the risk of desync
		float diff = (instSource->PositionF() - mConductor->SongTime);
		mConductor->SongTime += diff * 0.05f;
	}
	else
	{
		mConductor->SongTime = -mCountdownTimer;
	}

	mFadeToWhiteBaseTime = glm::max(mFadeToWhiteBaseTime, 0.001f);

	mFadeToWhiteTime += Stratum::gpGlobals->deltaTime;
	mFadeToWhiteTime = glm::min(mFadeToWhiteTime, mFadeToWhiteBaseTime);

	auto& whiteSprite = mScene->SpriteRenderers.Get(mWhiteSprite);

	if (mFadeToWhiteIntensity > 0.0f)
	{
		whiteSprite.SpriteColor.a = glm::mix(0.0f, mFadeToWhiteIntensity, mFadeToWhiteTime / mFadeToWhiteBaseTime);
	}
	else
	{
		whiteSprite.SpriteColor.a = glm::mix(1.0f, 0.0f, mFadeToWhiteTime / mFadeToWhiteBaseTime);
	}

	if (gGameState.DoBeatEveryNthBeat != 1)
	{
		if ((mConductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat == 0)
		{
			float mult = 1.0f;

			if (!DoBeat)
			{
				GuiZoomLevel += 0.035f * mult;
				ZoomLevel += 0.015f;
				DoBeat = true;
			}
		}
	}
	else
	{
		static uint32_t lastBeat = 0;

		if (lastBeat != mConductor->BeatCount)
		{
			lastBeat = mConductor->BeatCount;
			GuiZoomLevel += 0.035f;
			ZoomLevel += 0.015f;
			DoBeat = true;
		}

	}

	if (((mConductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat != 0 || gGameState.DoBeatEveryNthBeat == 1) && DoBeat)
		DoBeat = false;

	ZoomLevel = glm::mix(ZoomLevel, CameraZoomModifier, 5.0f * Stratum::gpGlobals->deltaTime);
	GuiZoomLevel = glm::mix(GuiZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);
	GuiZoomModifier = glm::mix(GuiZoomModifier, 0.0f, 2.5f * Stratum::gpGlobals->deltaTime);

	auto& mainCameraTransform = scene->Transforms.Get(MainCameraEntity);
	auto& mainCamera = scene->Cameras.Get(MainCameraEntity);
	auto& guiCameraTransform = scene->Transforms.Get(GuiCameraEntity);
	auto& guiCamera = scene->Cameras.Get(GuiCameraEntity);

	mainCamera.OrthographicZoom = 1.0f / ZoomLevel;
	guiCamera.OrthographicZoom = 1.0f / (GuiZoomLevel + GuiZoomModifier);

	if (TrackPlayersEnabled)
	{
		glm::vec2 target = {};
		uint32_t sectionIndex = glm::floor(mConductor->BeatCount / 4.0f);
		auto entity = 0U;

		if (sectionIndex < mConductor->chart.sections.size())
		{
			auto& section = mConductor->chart.sections[sectionIndex];
			if (section.mustHitSection)
			{
				target = mPlayerCharacter->CharaPosition + StageRegistry::GetCurrentStage()->Player.CameraOffset;
				entity = mPlayerCharacter->CharaEntity;
			}
			else
			{
				target = mOponentCharacter->CharaPosition + StageRegistry::GetCurrentStage()->Oponent.CameraOffset;
				entity = mOponentCharacter->CharaEntity;
			}
		}

		if (entity != 0)
		{
			auto& animator = mScene->SpriteAnimators.Get(entity);

			if (animator.CurrentAnimation.find("left") != std::string::npos)
				target.x -= 60.0f;

			if (animator.CurrentAnimation.find("right") != std::string::npos)
				target.x += 60.0f;

			if (animator.CurrentAnimation.find("down") != std::string::npos)
				target.y -= 60.0f;

			if (animator.CurrentAnimation.find("up") != std::string::npos)
				target.y += 60.0f;
		}

		gGameState.CameraPosition = glm::mix(gGameState.CameraPosition, target, 3.0f * Stratum::gpGlobals->deltaTime);
	}

	mainCameraTransform.SetPosition(glm::vec3(gGameState.CameraPosition, 0.0f));

	UpdateStage();
}

void Funkin::InGameSystem::PostUpdate(Stratum::Scene* scene)
{
	CharaRegistry::Update();
	bool before = mIsPaused;

	if (Stratum::Input::GetKeyDown(KeyCode::ESCAPE) || Stratum::Input::GetGamepadButtonDown(GamepadButton::START))
	{
		mIsPaused = !mIsPaused;
	}

	std::array<Stratum::ECS::edict_t, 6> entities{
		mResumeText,
		mVolumeText,
		mBotplayText,
		mBalatroText,
		mExitText,
		mSelectText
	};

	auto entity = Stratum::ECS::C_INVALID_ENTITY;

	auto& volumeSetting = Settings::s_Settings->Get("volume", 1.0f);

	if (mIsPaused)
	{
		if (Stratum::Input::GetKeyDown(KeyCode::DOWN) || Stratum::Input::GetGamepadButtonDown(GamepadButton::DPAD_DOWN))
		{
			mPauseUiButtonIndex += 1;
			scrollSource->Play();
		}
		if (Stratum::Input::GetKeyDown(KeyCode::UP) || Stratum::Input::GetGamepadButtonDown(GamepadButton::DPAD_UP))
		{
			mPauseUiButtonIndex -= 1;
			scrollSource->Play();
		}

		mPauseUiButtonIndex = glm::clamp(mPauseUiButtonIndex, 0, 4);

		switch (mPauseUiButtonIndex)
		{
		case 0:
			entity = mResumeText;
			break;
		case 1:
			entity = mVolumeText;
			break;
		case 2:
			entity = mBotplayText;
			break;
		case 3:
			entity = mBalatroText;
			break;
		case 4:
			entity = mExitText;
			break;
		default:
			break;
		}

		mScene->GuiAnchors.Get(mSelectText).Position.y = mScene->GuiAnchors.Get(entity).Position.y;

		if (Stratum::Input::GetKeyDown(KeyCode::RETURN) || Stratum::Input::GetGamepadButtonDown(GamepadButton::A))
		{
			if (mPauseUiButtonIndex == 0)
			{
				mIsPaused = false;
			}
			if (mPauseUiButtonIndex == 4)
			{
				auto scene = new Stratum::Scene();
				mScene->SwapScene(scene);
				scene->RegisterCustomSystem(new MainMenuSystem());
				//Stratum::EventBus::InvokeEvent(Stratum::ApplicationEvent{ Stratum::ApplicationEvent::APP_EVENT_SHUTDOWN });
			}
			if (mPauseUiButtonIndex == 2)
			{
				mConductor->BotPlay = !mConductor->BotPlay;
			}
			if (mPauseUiButtonIndex == 3)
			{
				auto scene = new Stratum::Scene();
				scene->RegisterCustomSystem(new BalatroSystem());
				mScene->SwapScene(scene);
				pauseSource->Stop();
			}
		}

		if (mPauseUiButtonIndex == 1)
		{
			if (Stratum::Input::GetKeyDown(KeyCode::LEFT) || 
				Stratum::Input::GetGamepadButtonDown(GamepadButton::DPAD_LEFT))
			{
				volumeSetting.floatValue -= 0.05f;
				scrollSource->Play();
				Settings::s_Settings->SaveToFile("settings.json");
			}
			if (Stratum::Input::GetKeyDown(KeyCode::RIGHT) || 
				Stratum::Input::GetGamepadButtonDown(GamepadButton::DPAD_RIGHT))
			{
				volumeSetting.floatValue += 0.05f;
				scrollSource->Play();
				Settings::s_Settings->SaveToFile("settings.json");
			}
			volumeSetting.floatValue = glm::clamp(volumeSetting.floatValue, 0.0f, 1.0f);
		}

		mScene->TextComponents.Get(mVolumeText).Text = std::format(L"volume: {:.0f}%", volumeSetting.floatValue * 100.0f);
		mScene->TextComponents.Get(mBotplayText).Text = std::format(L"Botplay: {}", mConductor->BotPlay);

		uint32_t index = 0;

		for (auto ent : entities)
		{
			if (ent != mSelectText)
			{
				float diff = 1.0f / (1.0f + (float)glm::abs((int)index - mPauseUiButtonIndex) / 10.0f * 3.0f);
				mScene->GuiAnchors.Get(ent).Position.y = glm::mix(mScene->GuiAnchors.Get(ent).Position.y, index * -100.0f + mPauseUiButtonIndex * 100.0f, Stratum::Time::UnscaledDeltaTime * 5.0f * diff);
				index++;
			}
		}
	}

	for (auto ent : entities)
	{
		mScene->TextRenderers.Get(ent).Enabled = mIsPaused;

		if (ent == entity)
		{
			mScene->TextComponents.Get(ent).FontSize = 100.0f;
		}
		else
		{
			mScene->TextComponents.Get(ent).FontSize = 90.0f;
		}
	}

	if (before != mIsPaused)
	{
		if (!before && mIsPaused) // Pausing
		{
			if (voicesSource)
				voicesSource->Pause();
			if (instSource)
				instSource->Pause();

			pauseSource->Play();

			mPauseUiButtonIndex = 0;

			for (auto ent : entities)
			{
				mScene->GuiAnchors.Get(ent).Position.y = -2000.0f;
			}
		}
		else if (before && !mIsPaused) // Unpausing
		{
			pauseSource->Stop();
			if (voicesSource)
			{
				voicesSource->Resume();
				voicesSource->Seek(instSource->PositionF());
			}
			if (instSource)
				instSource->Resume();

			mConductor->SongTime = instSource->PositionF();
		}
	}

	ma_engine_set_volume(mScene->AudioEngine->GetEngine(), volumeSetting.floatValue);

	mConductor->IsPaused = this->IsPaused();

	Stratum::Time::TimeScale = mIsPaused ? 0.0f : 1.0f;

	if (!instSource->IsPlaying() && !mIsPaused && mHasSongStarted)
	{
		mWaitTimer += Stratum::Time::DeltaTime;
		if (mWaitTimer > 1.0f)
		{
			auto scene = new Stratum::Scene();
			mScene->SwapScene(scene);
			scene->RegisterCustomSystem(new MainMenuSystem());
		}
	}
}

void Funkin::InGameSystem::RenderImGui(Stratum::Scene* scene)
{
	
}

void Funkin::InGameSystem::SetPlayerCharacter(CharaSprite* chara)
{
	if (mPlayerCharacter)
	{
		mPlayerCharacter->SetEnabled(false);
	}
	mPlayerCharacter = chara;
	mPlayerSprite = chara->CharaEntity;
	gGameState.pPlayerSystem->SetCharacter(chara);
	chara->SetEnabled(true);

	if (auto stage = StageRegistry::GetCurrentStage())
	{
		mPlayerCharacter->CharaPosition = stage->Player.Position;
		mPlayerCharacter->CharaScale = stage->Player.Scale;

		auto& sprite = mScene->SpriteRenderers.Get(chara->CharaEntity);
		sprite.RenderLayer = stage->Player.zIndex;
		sprite.CameraLayer = 0;
		sprite.FlipX = chara->FlippedHorizontally();
		sprite.SpriteColor = glm::vec4(1.0f);
		sprite.Rotation = {};
	}
}

void Funkin::InGameSystem::SetOpponentCharacter(CharaSprite* chara)
{
	if (mOponentCharacter)
	{
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		mOponentCharacter->SetEnabled(false);
	}

	mOponentCharacter = chara;

	if (!chara)
	{
		return;
	}

	chara->SetEnabled(true);

	if (auto stage = StageRegistry::GetCurrentStage())
	{
		mOponentCharacter->CharaPosition = stage->Oponent.Position;
		mOponentCharacter->CharaScale = stage->Oponent.Scale;

		auto& sprite = mScene->SpriteRenderers.Get(chara->CharaEntity);
		sprite.RenderLayer = stage->Oponent.zIndex;
		sprite.CameraLayer = 0;
		sprite.FlipX = !chara->FlippedHorizontally();
		sprite.SpriteColor = glm::vec4(1.0f);
		sprite.Rotation = {};
	}
}

Funkin::CharaSprite* Funkin::InGameSystem::GetPlayerCharacter()
{
	return mPlayerCharacter;
}

Funkin::CharaSprite* Funkin::InGameSystem::GetOpponentCharacter()
{
	return mOponentCharacter;
}

float Funkin::InGameSystem::GetLoadingProgress()
{
	return mLoadingStage.load() / 8.0f;
}

bool Funkin::InGameSystem::IsLoadingDone()
{
	return mLoadingDone.load();
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize, uint8_t cameraLayer, uint32_t renderLayer, float align)
{
	auto entity = mScene->EntityManager.CreateEntity();
	mScene->TextComponents.Create(entity);
	mScene->TextComponents.Get(entity).FontSize = fontSize;
	mScene->TextComponents.Get(entity).Text = defaultText;
	mScene->TextRenderers.Create(entity).Alignment = align;
	mScene->TextRenderers.Get(entity).RenderLayer = renderLayer;
	mScene->TextRenderers.Get(entity).CameraLayer = cameraLayer;
	mScene->Transforms.Create(entity);
	mScene->Transforms.Get(entity).SetPosition(glm::vec3(pos, 0.0f));
	return entity;
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateSpriteEntity(const::std::string& spritePath, const glm::vec2& pos, const glm::vec2& scale, uint8_t cameraLayer, uint32_t renderLayer, bool flipX)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.TextureHandle = mScene->Resources.LoadTextureImage(spritePath);
	sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.CameraLayer = cameraLayer;
	sprite.FlipX = flipX;
	sprite.Center = { 0.0f, 0.0f };
	
	transform.SetPosition(glm::vec3(pos, 1.0f));
	transform.SetScale(glm::vec3(scale, 1.0f));

	return entity;
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize, const glm::vec2& center, uint8_t cameraLayer, uint32_t renderLayer)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.Rect.size = rectSize;
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.CameraLayer = cameraLayer;
	sprite.Center = center;

	transform.SetPosition(glm::vec3(pos, 1.0f));

	return entity;
}

void Funkin::InGameSystem::UpdateStage()
{
	auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);

	auto& props = metadataManager->GetEntities();

	for (auto entity : props)
	{
		auto& metadata = metadataManager->Get(entity);
		auto& nameTag = mScene->Names.Get(entity);
		auto& transform = mScene->Transforms.Get(entity);
		auto& sprite = mScene->SpriteRenderers.Get(entity);

		glm::vec2 pos = metadata.Position;
		glm::vec2 scroll = metadata.Scroll;
		glm::vec2 targetPos = metadata.Position + gGameState.CameraPosition * scroll;

		if (nameTag.Name.starts_with("fb"))
		{
			sprite.SpriteColor = glm::vec4(glm::vec3(glm::abs(glm::sin(sprite.Rotation.x * 0.01f))) + 1.5f, 1.0f);
			if (sprite.Rotation.x == 0.0f)
			{
				sprite.Rotation.x = rand() % 360;
			}
			sprite.Rotation.x += (360.0f * mConductor->GetConductorBeatMultiplier() * 0.5f) * Stratum::gpGlobals->deltaTime;
		}

		transform.Position = glm::vec3(targetPos, 0.0f);

		transform.IsDirty = true;
	}
}
