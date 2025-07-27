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

Funkin::GameState gGameState;

#undef min
#undef max

float gMissTimer = 0.0f;

Funkin::InGameSystem::InGameSystem(const LoadChartParams& params) : mLoadParams(params)
{
	mLoadingDone.store(false);
}

Funkin::InGameSystem::~InGameSystem()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
}

static Stratum::ECS::edict_t startupVideo = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBar = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBarBg = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBarLoadedBg = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeDuration = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeHud = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeFade = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t loadingYoutubeEntity = Stratum::ECS::C_INVALID_ENTITY;


void Funkin::InGameSystem::Init(Stratum::Scene* scene)
{
	startupVideo = 0;
	gGameState = {};
	mScene = scene;

	StageRegistry::Init(mScene);
	CharaRegistry::Init(mScene);

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

	StageRegistry::AddStage("syobon1-4");
	StageRegistry::AddStage("syobon1-1");
	StageRegistry::AddStage("syobon-end");

	CharaRegistry::AddCharacter("Syobon");
	CharaRegistry::AddCharacter("syobon");
	CharaRegistry::AddCharacter("cebollaconpelo");
	CharaRegistry::AddCharacter("Fernan");
	CharaRegistry::AddCharacter("FernanRene");
	CharaRegistry::AddCharacter("FernanBebe");
	CharaRegistry::AddCharacter("FernanJumpeado");
	CharaRegistry::AddCharacter("rene");

	CharaRegistry::GetCharacter("Syobon")->AddAnimation("die", "Syobon Die", "", 30, true);

	CharaRegistry::GetCharacter("Fernan")->AddAnimation("leftAlt", "Fernan Left Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("rightAlt", "Fernan Right Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("upAlt", "Fernan Up Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("downAlt", "Fernan Down Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("ahh", "Fernan Goku2", "", 30, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues", "Fernan Cry3", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues1", "Fernan Cry1", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues2", "Fernan Cry2", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("WTF", "Fernan Left Alt", "", 8, false, true);

	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("no", "Fernan Empieza", "", 8, false, true);
	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("nO", "Fernan No0", "", 8, false, true);
	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("NO", "Fernan NOO", "", 8, false, true);

	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("eh", "Fernan Huh", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("nosabe", "Fernan Jejnosab", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("no", "Fernan Nono", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->SetCenter({});

	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("repeat", "Fernan Beberepeat", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("nopodemos", "Fernan Nopodemos", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("sipodemos", "Fernan Sipodemos", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("queno", "Fernan Queno", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("waa", "Fernan Waaa", "", 60, false);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("uahh", "Fernan UAHH", "", 60, true);

	StageRegistry::SetStage(mConductor->chart.info.stage);
	mLoadingStage.fetch_add(1);

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
	sprite.IsGui = true;
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

	mScene->FontRegistry.LoadFont("Funkin", "fonts/Phantomuff Difficult Font.ttf");
	mScene->FontRegistry.LoadFont("Youtube", "fonts/YoutubeSans-Titles.ttf");

	ChartEvent EventSyobon{};
	ChartEvent EventCebolla{};
	ChartEvent Event1dash4{};
	ChartEvent Event1dash2{};
	ChartEvent Event1dash1{};
	ChartEvent EventWhite{};
	ChartEvent EventStopWhite{};
	ChartEvent EventStartBlack{};
	ChartEvent EventStartBlack1{};
	ChartEvent EventStopBlack{};

	Event1dash4.EventName = "StSetStage";
	Event1dash2.EventName = "StSetStage";
	Event1dash1.EventName = "StSetStage";

	EventWhite.EventName = "StFadeToWhite";
	EventStopWhite.EventName = "StFadeToWhite";

	EventStartBlack.EventName = "StFadeToBlack";
	EventStartBlack1.EventName = "StFadeToBlack";
	EventStopBlack.EventName = "StFadeToBlack";

	EventSyobon.EventName = "StSetPlayerCharacter";
	EventCebolla.EventName = "StSetPlayerCharacter";

	EventSyobon.Arg1 = "syobon";
	EventCebolla.Arg1 = "cebollaconpelo";

	EventStartBlack1.EventTime = 109.75f;
	EventStartBlack.EventTime = 147.45f;
	EventStopBlack.EventTime = 149.65f;

	EventStartBlack.Arg1 = "1.0";
	EventStartBlack.Arg2 = "1";

	EventStartBlack1.Arg1 = "1.0";
	EventStartBlack1.Arg2 = "1";

	EventStopBlack.Arg1 = "0.0";
	EventStopBlack.Arg2 = "1";

	Event1dash4.EventTime = 110.82f;
	EventSyobon.EventTime = 110.82f;
	Event1dash2.EventTime = 149.65f;
	EventCebolla.EventTime = 149.65f;

	Event1dash1.EventTime = 232.49f;

	EventWhite.EventTime = 110.82f;
	EventStopWhite.EventTime = 110.9f;

	EventWhite.Arg1 = "1.0";
	EventWhite.Arg2 = "1";

	EventStopWhite.Arg1 = "0.0";
	EventStopWhite.Arg2 = "1000";

	Event1dash2.Arg1 = "syobon";
	Event1dash4.Arg1 = "syobon1-4";
	Event1dash1.Arg1 = "syobon1-1";

	mConductor->chart.events.push_back(Event1dash4);
	mConductor->chart.events.push_back(Event1dash2);
	mConductor->chart.events.push_back(EventWhite);
	mConductor->chart.events.push_back(EventStopWhite);
	mConductor->chart.events.push_back(EventStartBlack1);
	mConductor->chart.events.push_back(EventStartBlack);
	mConductor->chart.events.push_back(EventStopBlack);
	mConductor->chart.events.push_back(EventSyobon);
	mConductor->chart.events.push_back(EventCebolla);
	mConductor->chart.events.push_back(Event1dash1);

	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen2.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/holasaul.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/loadingYoutube.png");

	static Stratum::ECS::edict_t x64entity;
	static Stratum::ECS::edict_t x64blackentity;

	youtubeDuration = CreateTextEntity(L"0:00", { 0.0f, 0.0f }, 50.0f, true, 901, 0.0f);
	auto& youtubeAnchor = mScene->GuiAnchors.Create(youtubeDuration);
	youtubeAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeAnchor.Position = { 500, 67.0f };

	youtubeHud = CreateSpriteEntity("fnf/SyobonNoAction/videohud.png", { 0.0f, 0.0f }, { 1.0f, 1.0f }, true, 900);
	youtubeFade = CreateSpriteEntity("fnf/SyobonNoAction/black.png", { 0.0f, 0.0f }, { 1.0f, 1.0f }, true, 899);
	youtubeBar = CreateRectEntity({ 0.0f, 0.0f }, { 100.0f, 10.0f }, { -1.0f, 1.0f }, true, 902);
	youtubeBarBg = CreateRectEntity({ 0.0f, 0.0f }, { 100.0f, 10.0f }, { -1.0f, 1.0f }, true, 901);

	auto youtubeTitle = CreateTextEntity(L"Gato Bros (Syobon Action) en español por fernanfloo", { 0.0f, 0.0f }, 70.0f, true, 900, 0.0f);
	auto& youtubeTitleAnchor = mScene->GuiAnchors.Create(youtubeTitle);
	youtubeTitleAnchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP_LEFT;
	youtubeTitleAnchor.Position = { 35.0f, 100.0f };
	mScene->TextComponents.Get(youtubeTitle).Font = "Youtube";

	auto& transform = scene->Transforms.Get(youtubeBar);
	transform.Scale.x = 0.0f;

	auto& youtubeBarAnchor = mScene->GuiAnchors.Create(youtubeBar);
	youtubeBarAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeBarAnchor.Position = { 25.0f, 165.0f };

	auto& youtubeBarBgAnchor = mScene->GuiAnchors.Create(youtubeBarBg);
	youtubeBarBgAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeBarBgAnchor.Position = { 25.0f, 165.0f };

	mScene->SpriteRenderers.Get(youtubeBar).SpriteColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor = { 0.3f, 0.3f, 0.3f, 0.7f };

	{
		mScene->SpriteRenderers.Get(youtubeBar).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeHud).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeFade).SpriteColor.a = 0.0f;
		mScene->TextRenderers.Get(youtubeTitle).Color.a = 0.0f;
		mScene->TextRenderers.Get(youtubeDuration).Color.a = 0.0f;

		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeBar).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor.a, 0.7f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeHud).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeFade).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->TextRenderers.Get(youtubeTitle).Color.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->TextRenderers.Get(youtubeDuration).Color.a, 1.0f, 2.0f, Easing::Linear);
	}

	mConductor->AddScriptedEvent(0, [this]()
		{
			startupVideo = mScene->EntityManager.CreateEntity();
			auto& sprite = mScene->SpriteRenderers.Create(startupVideo);
			auto& transform = mScene->Transforms.Create(startupVideo);

			auto& surface = mScene->VideoSurfaces.Create(startupVideo);
			surface.Path = "fnf/videos/GatoBros.mp4";
			mScene->InitVideo(surface);

			surface.SetPlayState(true);

			sprite.Rect.position = {};
			sprite.Rect.size = surface.VideoResolution;
			sprite.IsGui = true;
			sprite.RenderLayer = 101;
			sprite.TextureHandle = surface.TextureHandle;
			sprite.SpriteColor.r = 0.0f;
			sprite.SpriteColor.g = 0.0f;
			sprite.SpriteColor.b = 0.0f;
			transform.SetScale(glm::vec3(20.0f));

			gGameState.DoBeatEveryNthBeat = 999999999;

			auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
			auto& props = metadataManager->GetEntities();

			for (auto entity : props)
			{
				auto& metadata = metadataManager->Get(entity);
				auto& nameTag = mScene->Names.Get(entity);
				auto& transform = mScene->Transforms.Get(entity);
				auto& sprite = mScene->SpriteRenderers.Get(entity);

				if (nameTag.Name.starts_with("Muelto"))
				{
					metadata.Position.y += 1520.0f;
				}
			}

			CharaSprite* chara;
			CharaSprite* lastChara = mOponentCharacter;
			this->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("Syobon"));
			chara->CharaPosition = { -156.0f, 76.0f };
			chara->CharaScale = { 0.825f, 0.825f };
			this->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
			chara->SetEnabled(true);
		});
	/*
	mConductor->AddScriptedEvent(2, [this]()
		{
			loadingYoutubeEntity = mScene->EntityManager.CreateEntity();
			auto& animator = mScene->SpriteAnimators.Create(loadingYoutubeEntity);
			auto& sprite = mScene->SpriteRenderers.Create(loadingYoutubeEntity);
			auto& transform = mScene->Transforms.Create(loadingYoutubeEntity);

			auto frames = SparrowReader::readXML("fnf/SyobonNoAction/loadingYoutube.xml", "loading", false, false);

			Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(60)
				.SetLoop(true)
				.SetAnimateOnIdle(true)
				.SetFrames(frames);

			animator.AnimationMap["loading"] = animation;
			animator.SetState("loading");

			sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/loadingYoutube.png");
			sprite.RenderLayer = 105;
			sprite.IsGui = true;
			sprite.Center = { 0.0f, 0.0f };

		});
	*/
	mConductor->AddScriptedEvent(257, [this]() {

		x64entity = mScene->EntityManager.CreateEntity();
		x64blackentity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(x64entity);
		auto& transform = mScene->Transforms.Create(x64entity);
		auto& sprite1 = mScene->SpriteRenderers.Create(x64blackentity);
		auto& transform1 = mScene->Transforms.Create(x64blackentity);

		mScene->EntityManager.DestroyEntity(startupVideo);
		startupVideo = Stratum::ECS::C_INVALID_ENTITY;

		sprite1.SpriteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		sprite1.Rect.size = { 100000, 100000 };
		sprite1.IsGui = true;
		sprite1.RenderLayer = 101;
		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = true;
		sprite.RenderLayer = 102;
		sprite.IsGui = true;
		transform.SetScale(glm::vec3(1.25f));

		});
	mConductor->AddScriptedEvent(272, [this]() {
		mScene->EntityManager.DestroyEntity(x64blackentity);
		mScene->EntityManager.DestroyEntity(x64entity);
		gGameState.DoBeatEveryNthBeat = 2;
		});
	mConductor->AddScriptedEvent(773, [this]() {
		CharaSprite* chara;
		CharaSprite* lastChara = mOponentCharacter;
		this->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("Syobon"));
		chara->CharaPosition = { -156.0f, 76.0f };
		chara->CharaScale = { 0.825f, 0.825f };
		lastChara->SetEnabled(true);
		});
	mConductor->AddScriptedEvent(1536, [this]() {
		CharaSprite* sprite;
		this->SetPlayerCharacter(sprite = CharaRegistry::GetCharacter("syobon"));
		sprite->CharaPosition = {};
		mScene->SpriteRenderers.Get(sprite->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(sprite->CharaEntity).IsGui = true;
		});
	mConductor->AddScriptedEvent(1550, [this]() {
		mConductor->EnableBot = true;
		Stratum::VarRegistry::GetConsoleVar("r", "post_ca_enabled")->set(true);
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1174, [this]() {
		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Muelto"))
			{
				pTimedActionSystem->PushAction(&metadata.Position.y, metadata.Position.y - 1520.0f, mConductor->StepsToSeconds(3), Easing::Linear);
			}
		}
		});
	mConductor->AddScriptedEvent(1177, [this]() {
		CharaSprite* lastChara = mOponentCharacter;
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		lastChara->SetEnabled(true);
		lastChara->PlayAnimation("die");
		pTimedActionSystem->PushAction(&lastChara->CharaPosition.y, lastChara->CharaPosition.y - 2500.0f * 1.5f, 1.2f * 1.25f, Easing::BackIn);
		});
	mConductor->AddScriptedEvent(1200, [this]() {
		mOponentCharacter->CharaPosition.y += 200;
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		sprite.Center = {};
		pTimedActionSystem->PushAction(&sprite.Rotation.x, 30, ActionParameters(0.9f, 28.0f), Easing::Sine);
		});
	mConductor->AddScriptedEvent(1218, [this]() {
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, -3600, 1.0f, Easing::SineIn);
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaPosition.y, -3600, 1.0f, Easing::SineIn);
		});
	mConductor->AddScriptedEvent(1270, [this]() {
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("FernanBebe"));
		CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y += 1600;
		CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.x = -CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.x;
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y,CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y - 1600, 2.0f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(1356, [this]() {
		mOponentCharacter->PlayAnimation("uahh");
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x, CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x + 1.5f, 0.15f, Easing::Linear);
		});
	mConductor->AddScriptedEvent(1360, [this]() {
		CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x -= 1.5f;
		mOponentCharacter->PlayAnimation("repeat");
		});
	mConductor->AddScriptedEvent(1379, [this]() {
		mOponentCharacter->PlayAnimation("nopodemos");
		});
	mConductor->AddScriptedEvent(1391, [this]() {
		mOponentCharacter->PlayAnimation("sipodemos");
		});
	mConductor->AddScriptedEvent(1400, [this]() {
		mOponentCharacter->PlayAnimation("queno");
		});
	mConductor->AddScriptedEvent(1407, [this]() {
		mOponentCharacter->PlayAnimation("sipodemos");
		});
	mConductor->AddScriptedEvent(1415, [this]() {
		mOponentCharacter->PlayAnimation("waa");
		});
	mConductor->AddScriptedEvent(1420, [this]() {
		auto manager = mScene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);
		auto entity = mScene->EntityManager.CreateEntity();
		auto& transform = mScene->Transforms.Create(entity);
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& animator = mScene->SpriteAnimators.Create(entity);
		manager->Create(entity);

		transform.SetPosition(glm::vec3(969.000, 609.000, 0.0f));

		auto frames = SparrowReader::readXML("fnf/SyobonNoAction/holasaul.xml", "holasaul", false);

		Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(30)
			.SetLoop(false)
			.SetNextState("destroy")
			.SetFrames(frames);

		animator.AnimationMap["saul"] = animation;
		animator.SetState("saul");

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/holasaul.png");
		sprite.RenderLayer = 10000;
		});
	mConductor->AddScriptedEvent(1536, [this]() {
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity).IsGui = true;
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		sprite.Center = { 0.0f, 1.0f };

		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Muelto"))
			{
				metadata.Position.y += 15200.0f;
			}
		}

		});
	mConductor->AddScriptedEvent(1547, [this]() {
		CharaRegistry::GetCharacter("Fernan")->PlayAnimation("WTF");
		});
	mConductor->AddScriptedEvent(1551, [this]() {
		mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity).IsGui = false;
		});
	mConductor->AddScriptedEvent(2089, [this]() {
		CharaSprite* chara;
		this->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("rene"));
		chara->CharaPosition = { -746.0f, -212.0f };
		chara->CharaScale = { 0.825f, 0.825f };
		Stratum::VarRegistry::GetConsoleVar("r", "post_ca_enabled")->set(false);
		});
	mConductor->AddScriptedEvent(2613, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->SetEnabled(true);
		CharaRegistry::GetCharacter("FernanRene")->SetLayer(1000);
		CharaRegistry::GetCharacter("FernanRene")->CharaPosition = { mScene->VirtualScreenSize.x + 9000.0f, 0.0f };
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanRene")->CharaPosition, glm::vec2(0.0f), 0.13f, Easing::Linear);
		});
	mConductor->AddScriptedEvent(2615, [this]() {
		auto oponent = CharaRegistry::GetCharacter("rene");
		mScene->SpriteRenderers.Get(oponent->CharaEntity).SpriteColor = glm::vec4(1.5f, 0.0f, 0.0f, 1.0f);
		pTimedActionSystem->PushAction(&oponent->CharaPosition, glm::vec2(oponent->CharaPosition.x - mScene->VirtualScreenSize.x / 2.0f, oponent->CharaPosition.y), 0.2f, Easing::ElasticInOut,
			[this]() {
				auto oponent = CharaRegistry::GetCharacter("rene");
				auto& sprite = mScene->SpriteRenderers.Get(oponent->CharaEntity);
				mScene->SpriteRenderers.Get(oponent->CharaEntity).SpriteColor = glm::vec4(1.5f, 0.0f, 0.0f, 1.0f);
				pTimedActionSystem->PushAction(&sprite.Rotation.x, 360, 0.8f, Easing::Random);
				pTimedActionSystem->PushAction(&oponent->CharaScale, oponent->CharaScale + glm::vec2(0.75f, 0), 0.8f, Easing::Random,
					[this]() {
						auto oponent = CharaRegistry::GetCharacter("rene");
						auto& sprite = mScene->SpriteRenderers.Get(oponent->CharaEntity);
						pTimedActionSystem->PushAction(&oponent->CharaPosition, glm::vec2(oponent->CharaPosition.x - 1000.0f, oponent->CharaPosition.y), 0.4f, Easing::Random);
						pTimedActionSystem->PushAction(&sprite.Rotation.x, -360, 0.8f, Easing::Random);
					});
			});
		});
	mConductor->AddScriptedEvent(2648, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2676, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2678, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2680, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2682, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2684, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2698, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("nosabe");
		});
	mConductor->AddScriptedEvent(2700, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2704, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("nosabe");
		});
	mConductor->AddScriptedEvent(2720, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->SetEnabled(false);
		x64entity = mScene->EntityManager.CreateEntity();
		x64blackentity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(x64entity);
		auto& transform = mScene->Transforms.Create(x64entity);
		auto& sprite1 = mScene->SpriteRenderers.Create(x64blackentity);
		auto& transform1 = mScene->Transforms.Create(x64blackentity);

		sprite1.SpriteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		sprite1.Rect.size = { 100000, 100000 };
		sprite1.IsGui = true;
		sprite1.RenderLayer = 101;
		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen2.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = true;
		sprite.RenderLayer = 102;
		sprite.IsGui = true;
		transform.SetScale(glm::vec3(0.25f));
		});
	mConductor->AddScriptedEvent(2727, [this]() {
		mScene->EntityManager.DestroyEntity(x64blackentity);
		mScene->EntityManager.DestroyEntity(x64entity);
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("FernanJumpeado"));
		CharaRegistry::GetCharacter("FernanJumpeado")->CharaPosition.x = -CharaRegistry::GetCharacter("FernanJumpeado")->CharaPosition.x;
		});
	mConductor->AddScriptedEvent(2958, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2970, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("nO");
		});
	mConductor->AddScriptedEvent(2976, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("NO");
		});
	mConductor->AddScriptedEvent(2982, [this]() {
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaScale, mOponentCharacter->CharaScale + glm::vec2(2.5f, 0.0f), 0.2f, Easing::SineInOut
			, [this]() 
			{
				mOponentCharacter->SetEnabled(false);
			});
		});
	mConductor->AddScriptedEvent(3108, [this]() {
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		CharaRegistry::GetCharacter("Fernan")->CharaPosition.x -= 1000.0f;
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("Fernan")->CharaPosition.x, CharaRegistry::GetCharacter("Fernan")->CharaPosition.x + 1000.0f, 0.3f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(3239, [this]() {
		CharaRegistry::GetCharacter("Fernan")->PlayAnimation("ahh");
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("Fernan")->CharaPosition, glm::vec2(0.0f, -200), 0.5f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(3255, [this]() {
		this->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		CharaRegistry::GetCharacter("Fernan")->SetEnabled(true);
		mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity).RenderLayer = 10000;
		});
	mConductor->AddScriptedEvent(3784, [this]() {
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaPosition, mOponentCharacter->CharaPosition + glm::vec2(160, -160), 0.35f, Easing::SineOut,
			[this]() {
				pTimedActionSystem->PushAction(&mOponentCharacter->CharaPosition, mOponentCharacter->CharaPosition + glm::vec2(160, -160), 0.35f, Easing::SineOut);
			});
		});
	mConductor->AddScriptedEvent(3800, [this]() {
		mOponentCharacter->CharaPosition.y += 200;
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaPosition, mOponentCharacter->CharaPosition - glm::vec2(320, -320), 0.35f, Easing::SineOut);
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, 30.0f, ActionParameters(1.3f, 30.0f), Easing::Sine);
		sprite.Center = {};
		});
	mConductor->AddScriptedEvent(3835, [this]() {
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, -20.0f, 1.7f * 4.0, Easing::SineInOut);
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaScale, mOponentCharacter->CharaScale + glm::vec2(0.6f, 0.0f), 1.7f * 3.0f, Easing::SineInOut);
		pTimedActionSystem->PushAction(&mOponentCharacter->CharaPosition, mOponentCharacter->CharaPosition - glm::vec2(0, 1200 * 2.0f), 1.7f * 3.0, Easing::SineInOut);
		});
	mConductor->AddScriptedEvent(3856, [this]() {
		mOponentCharacter->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3857, [this]() {
		mOponentCharacter->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3858, [this]() {
		mOponentCharacter->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3859, [this]() {
		mOponentCharacter->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(4056, [this]() {
		pTimedActionSystem->ClearActions();
		StageRegistry::SetStage("syobon-end");
		SetPlayerCharacter(CharaRegistry::GetCharacter("Syobon"));
		SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));

		mOponentCharacter->SetCenter({ 0.0f, 1.0f });
		mOponentCharacter->SetEnabled(false);

		mScene->SpriteRenderers.Get(this->GetPlayerCharacter()->CharaEntity).FlipX = false;
		});
	mConductor->AddScriptedEvent(4074, [this]() {
		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Tb"))
			{
				pTimedActionSystem->PushAction(&metadata.Position.y, metadata.Position.y - 1000.0f, mConductor->StepsToSeconds(6), Easing::SineIn);
			}
		}

		CharaSprite* chara = this->GetPlayerCharacter();

		pTimedActionSystem->PushAction(&chara->CharaPosition.y, chara->CharaPosition.y - 1000.0f, mConductor->StepsToSeconds(6), Easing::SineIn);

		});
	mConductor->AddScriptedEvent(4082, [this]() {
		mOponentCharacter->SetEnabled(true);
		mOponentCharacter->PlayAnimation("llorapues");
		});


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
		scene->TextRenderers.Get(entity).IsGui = true;
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
		scene->TextRenderers.Get(entity).IsGui = true;
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
		scene->TextRenderers.Get(entity).IsGui = true;
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
		scene->TextRenderers.Get(entity).IsGui = true;
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
		scene->TextRenderers.Get(entity).IsGui = true;
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
		scene->TextRenderers.Get(entity).IsGui = true;
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
			voicesSource->SetVolume(0.2f);
			missSources[rand() % 3]->Play();
			gMissTimer = 0.4f;
		};
	auto hitListener = [this](const NoteEvent& e)
		{
			if (e.IsMiss || e.IsOponent)
				return;
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

	if (Stratum::ECS::C_INVALID_ENTITY != loadingYoutubeEntity)
	{
		auto& transform = mScene->Transforms.Get(loadingYoutubeEntity);
		auto& sprite = mScene->SpriteRenderers.Get(loadingYoutubeEntity);
		auto& animator = mScene->SpriteAnimators.Get(loadingYoutubeEntity);
		
		transform.Position.x = -121;
		transform.IsDirty = true;
		//transform.Position.y = -animator.GetCurrentRect().FrameSize.y / 2;

	}

	if (!mHasSongStarted)
	{
		mHasSongStarted = true;
		instSource->Play();
		if (voicesSource)
			voicesSource->Play();
	}

	if (mConductor->BeatCountF < 3.0f)
	{
		if (voicesSource)
			voicesSource->SetVolume(1.0f);
		instSource->SetVolume(1.0f);

	}

	if (Stratum::Input::GetKeyDown(KeyCode::F11))
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

	if (startupVideo != Stratum::ECS::C_INVALID_ENTITY)
	{
		auto& transform = scene->Transforms.Get(startupVideo);
		auto& sprite = scene->SpriteRenderers.Get(startupVideo);
		glm::vec2 scaleFactor = glm::vec2(scene->VirtualScreenSize) / glm::vec2(sprite.Rect.size);
		transform.SetScale(glm::vec3(scaleFactor, 0.0f));

		sprite.SpriteColor.r += 0.1f * Stratum::gpGlobals->deltaTime;
		sprite.SpriteColor.g += 0.1f * Stratum::gpGlobals->deltaTime;
		sprite.SpriteColor.b += 0.1f * Stratum::gpGlobals->deltaTime;

		sprite.SpriteColor = glm::min(sprite.SpriteColor, glm::vec4(1.0f));
	}

	float bpmPerSecond = 1.0f / (mConductor->chart.info.bpm / 60.0f);
	static float songDeltaTime = 0.0f;
	static float lastSongTime = 0.0f;
	static glm::vec3 bfPosition = glm::vec3(0.0f);

	static float GuiZoomLevel = 1.0f;
	static float ZoomLevel = 1.0f;
	static bool DoBeat = false;

	mConductor->SongTime = instSource->PositionF();

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

	ZoomLevel = glm::mix(ZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);
	GuiZoomLevel = glm::mix(GuiZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);

	scene->RenderPath3D->RenderPath2D->SetGuiCameraZoom({ GuiZoomLevel, GuiZoomLevel });
	scene->RenderPath3D->RenderPath2D->SetCameraZoom({ ZoomLevel, ZoomLevel });

	scene->RenderPath3D->RenderPath2D->SetCameraPosition(gGameState.CameraPosition);

	UpdateStage();

	uint32_t seconds = voicesSource->PositionF();

	{
		auto& sprite = scene->SpriteRenderers.Get(youtubeHud);
		glm::vec2 scaleFactor = glm::vec2(scene->VirtualScreenSize) / glm::vec2(sprite.Rect.size + glm::ivec2(67, 30));
		mScene->Transforms.Get(youtubeHud).SetScale(glm::vec3(scaleFactor, 0.0f));
	}
	{
		auto& sprite = scene->SpriteRenderers.Get(youtubeFade);
		glm::vec2 scaleFactor = glm::vec2(scene->VirtualScreenSize) / glm::vec2(sprite.Rect.size);
		mScene->Transforms.Get(youtubeFade).SetScale(glm::vec3(scaleFactor, 0.0f));
	}
	{
		auto& sprite = scene->SpriteRenderers.Get(youtubeBar);
		auto& sprite1 = scene->SpriteRenderers.Get(youtubeBarBg);
		auto& transform = scene->Transforms.Get(youtubeBar);
		transform.Scale.x = glm::mix(transform.Scale.x, (float)seconds / 291.0f, Stratum::Time::DeltaTime);
		transform.IsDirty = true;
		sprite.Rect.size.x = (scene->VirtualScreenSize.x * 2.0f - 50.0f);
		sprite1.Rect.size.x = (scene->VirtualScreenSize.x * 2.0f - 50.0f);
	}

	mScene->TextComponents.Get(youtubeDuration).Text = Stratum::Utils::FormatString(L"{}:{:02d} / 4:51", seconds / 60, seconds % 60);
}

void Funkin::InGameSystem::PostUpdate(Stratum::Scene* scene)
{
	CharaRegistry::Update();
	bool before = mIsPaused;

	if (Stratum::Input::GetKeyDown(KeyCode::ESCAPE))
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

	if (mIsPaused)
	{
		if (Stratum::Input::GetKeyDown(KeyCode::DOWN))
		{
			mPauseUiButtonIndex += 1;
			scrollSource->Play();
		}
		if (Stratum::Input::GetKeyDown(KeyCode::UP))
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

		if (Stratum::Input::GetKeyDown(KeyCode::RETURN))
		{
			if (mPauseUiButtonIndex == 0)
			{
				mIsPaused = false;
			}
			if (mPauseUiButtonIndex == 4)
			{
				Stratum::EventBus::InvokeEvent(Stratum::ApplicationEvent{ Stratum::ApplicationEvent::APP_EVENT_SHUTDOWN });
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
			if (Stratum::Input::GetKeyDown(KeyCode::LEFT))
			{
				mVolume -= 0.05f;
				scrollSource->Play();
			}
			if (Stratum::Input::GetKeyDown(KeyCode::RIGHT))
			{
				mVolume += 0.05f;
				scrollSource->Play();
			}
			mVolume = glm::clamp(mVolume, 0.0f, 1.0f);
		}

		mScene->TextComponents.Get(mVolumeText).Text = std::format(L"volume: {:.0f}%", mVolume * 100.0f);
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
				voicesSource->Seek(voicesSource->PositionF());
			}
			if (instSource)
				instSource->Resume();
		}
	}

	ma_engine_set_volume(mScene->AudioEngine->GetEngine(), mVolume);

	mConductor->IsPaused = this->IsPaused();

	Stratum::Time::TimeScale = mIsPaused ? 0.0f : 1.0f;

	if (!instSource->IsPlaying() && !mIsPaused)
	{
		mWaitTimer += Stratum::Time::DeltaTime;
		if (mWaitTimer > 1.0f)
		{
			auto scene = new Stratum::Scene();
			scene->RegisterCustomSystem(new BalatroSystem());
			mScene->SwapScene(scene);
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
		sprite.IsGui = false;
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
		sprite.IsGui = false;
		sprite.FlipX = !chara->FlippedHorizontally();
		sprite.SpriteColor = glm::vec4(1.0f);
		sprite.Rotation = {};
	}
}

Funkin::CharaSprite* Funkin::InGameSystem::GetPlayerCharacter()
{
	return mPlayerCharacter;
}

float Funkin::InGameSystem::GetLoadingProgress()
{
	return mLoadingStage.load() / 8.0f;
}

bool Funkin::InGameSystem::IsLoadingDone()
{
	return mLoadingDone.load();
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize, bool isGui, uint32_t renderLayer, float align)
{
	auto entity = mScene->EntityManager.CreateEntity();
	mScene->TextComponents.Create(entity);
	mScene->TextComponents.Get(entity).FontSize = fontSize;
	mScene->TextComponents.Get(entity).Text = defaultText;
	mScene->TextRenderers.Create(entity).Alignment = align;
	mScene->TextRenderers.Get(entity).RenderLayer = renderLayer;
	mScene->TextRenderers.Get(entity).IsGui = isGui;
	mScene->Transforms.Create(entity);
	mScene->Transforms.Get(entity).SetPosition(glm::vec3(pos, 0.0f));
	return entity;
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateSpriteEntity(const::std::string& spritePath, const glm::vec2& pos, const glm::vec2& scale, bool isGui, uint32_t renderLayer, bool flipX)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.TextureHandle = mScene->Resources.LoadTextureImage(spritePath);
	sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.IsGui = isGui;
	sprite.FlipX = flipX;
	sprite.Center = { 0.0f, 0.0f };
	
	transform.SetPosition(glm::vec3(pos, 1.0f));
	transform.SetScale(glm::vec3(scale, 1.0f));

	return entity;
}

Stratum::ECS::edict_t Funkin::InGameSystem::CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize, const glm::vec2& center, bool isGui, uint32_t renderLayer)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.Rect.size = rectSize;
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.IsGui = isGui;
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
