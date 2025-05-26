#include "InGameSystem.h"

#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"
#include "Player.h"

#include "StageEditorSystem.h"
#include "CharaEditorSystem.h"

#include "StageRegistry.h"
#include "CharaRegistry.h"

#include <Core/EngineStats.h>
#include <Core/Time.h>
#include <Core/Window.h>
#include <Core/JobManager.h>

#include <Media/VideoDecode.h>

#include <Util/Globals.h>
#include <Event/EventHandler.h>
#include <Input/Input.h>
#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>

#include <Renderer/GraphicsCommandBuffer.h>

#include <Thirdparty/imgui/imgui.h>
#include <json/json.hpp>


Funkin::GameState gGameState;

#undef min
#undef max

using namespace Stratum;

struct SpriteVideo
{
	ECS::edict_t entity;
	VideoDecode* decode;
	VideoParams params;
	float accum = 0.0f;
	float time = 0.0f;
	bool firstFrameReady = false;
	Ref<Render::GraphicsCommandBuffer> commandbuffer;
	Render::ImageResource* surface;
	DescriptorHandle handle;

	SpriteVideo(const std::string& path, SceneResources* resources)
	{
		decode = new Stratum::VideoDecode(path.c_str(), NULL);
		commandbuffer = CreateRef<Render::GraphicsCommandBuffer>();
		params = decode->GetSize();

		Render::ImageDescription desc;
		desc.Width = params.width;
		desc.Height = params.height;
		desc.Format = Render::ImageFormat::RGBA8_UNORM;

		handle = resources->CreateTextureImage(desc);
		surface = resources->GetImageHandle(handle);
	}
	~SpriteVideo()
	{
		delete decode;
	}
	void Update(Scene* scene)
	{
		auto& sprite = scene->SpriteRenderers.Get(entity);
		accum += Stratum::gpGlobals->deltaTime; if (decode->Finished())
		{
			sprite.Enabled = false;
			return;
		}
		float frameTime = decode->GetFrametime();
		decode->Step();
		while (accum >= frameTime && !decode->Finished())
		{
			accum -= frameTime;
			auto frame = decode->GetFrame();
			if (frame)
			{
				commandbuffer->Begin();
				auto cmd = commandbuffer->GetNativeCommandList();
				commandbuffer->RequireTextureState(surface, Render::ResourceState::ShaderResource, Render::ResourceState::CopyDest);
				commandbuffer->CommitBarriers();

				uint32_t count = params.width * params.height;
				auto data = frame->native()->data[0];

				JobManager::Dispatch(count, 1024, [&](JobDispatchArgs args)
					{
						uint32_t index = args.jobIndex * 4;
						uint8_t r = data[index + 0];
						uint8_t g = data[index + 1];
						uint8_t b = data[index + 2];
						uint8_t a = data[index + 3];
						data[index + 0] = g;
						data[index + 1] = b;
						data[index + 2] = a;
						data[index + 3] = r;
					});

				JobManager::Wait();

				cmd->writeTexture(surface->Handle, 0, 0, data, params.width * 4);
				commandbuffer->RequireTextureState(surface, Render::ResourceState::CopyDest, Render::ResourceState::ShaderResource);
				commandbuffer->CommitBarriers();
				commandbuffer->End();
				commandbuffer->Submit();
				decode->PushFrame(frame);
			}
		}
		auto& transform = scene->Transforms.Get(entity);
		glm::vec2 scaleFactor = glm::vec2(scene->VirtualScreenSize) / glm::vec2(sprite.Rect.size);
		transform.SetScale(glm::vec3(scaleFactor, 0.0f));

		time += gpGlobals->deltaTime;

		if (time > 3.5f)
		{
			sprite.SpriteColor.r += 0.1f * gpGlobals->deltaTime;
			sprite.SpriteColor.g += 0.1f * gpGlobals->deltaTime;
			sprite.SpriteColor.b += 0.1f * gpGlobals->deltaTime;
		}

	}
	void Create(Scene* scene)
	{
		entity = scene->EntityManager.CreateEntity();
		auto& sprite = scene->SpriteRenderers.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		sprite.Rect.position = {};
		sprite.Rect.size = { params.width, params.height };
		sprite.IsGui = true;
		sprite.RenderLayer = 101;
		sprite.TextureHandle = handle;
		sprite.SpriteColor.r = 0.0f;
		sprite.SpriteColor.g = 0.0f;
		sprite.SpriteColor.b = 0.0f;
		transform.SetScale(glm::vec3(20.0f));
	}
};

static SpriteVideo* startUpVideo;

Funkin::InGameSystem::InGameSystem(const LoadChartParams& params) : mLoadParams(params)
{
	
}

Funkin::InGameSystem::~InGameSystem()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
	delete startUpVideo;
}

void Funkin::InGameSystem::Init(Stratum::Scene* scene)
{
	gGameState = {};
	mScene = scene;

	StageRegistry::Init(mScene);
	CharaRegistry::Init(mScene);

	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteHoldComponent>(), C_NOTE_HOLD_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<AnimatedEffectComponent>(), C_ANIMATED_EFFECT_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<StagePropComponent>(), C_STAGE_PROP_COMPONENT_NAME);

	mConductor = new Conductor();
	gGameState.pConductor = mConductor;
	gGameState.pInGame = this;

	PlayerSystem* playerSystem;

	mScene->RegisterCustomSystem(mConductor, true);
	mScene->RegisterCustomSystem(playerSystem = new PlayerSystem(mConductor, &gGameState), true);

	gGameState.pPlayerSystem = playerSystem;

	mConductor->LoadChart(mScene, mLoadParams.ChartPath);

	if (!mLoadParams.OverrideStage.empty())
		mConductor->chart.info.stage = mLoadParams.OverrideStage;

	if (!mLoadParams.OverridePlayer1.empty())
		mConductor->chart.info.player1 = mLoadParams.OverridePlayer1;

	StageRegistry::AddStage(mConductor->chart.info.stage);
	CharaRegistry::AddCharacter(mConductor->chart.info.player1);

	StageRegistry::AddStage("syobon1-4");
	StageRegistry::AddStage("syobon1-1");

	CharaRegistry::AddCharacter("syobon");
	CharaRegistry::AddCharacter("cebollaconpelo");

	StageRegistry::SetStage(mConductor->chart.info.stage);

	std::string instPath = C_SONG_PATH_PREFIX;
	std::string voicesPath = "fnf/songs/";
	instPath.append(mConductor->chart.info.song).append("/Inst.mp3");
	voicesPath.append(mConductor->chart.info.song).append("/Voices.mp3");

	this->SetPlayerCharacter(CharaRegistry::GetCharacter(mConductor->chart.info.player1));

	for (int i = 0; i < 3; i++)
	{
		std::string path = "fnf/sounds/missnote";
		path.append(std::to_string(i + 1)).append(".mp3");
		missSources[i] = Stratum::CreateRef<Stratum::MP3AudioSource>(path.c_str(), scene->AudioEngine->GetEngine());
		missSources[i]->SetVolume(0.25f);
		scene->AudioEngine->AddSource(missSources[i]);
	}

	auto missListener = [this](void* sender, void** args, uint32_t argc)
		{
			voicesSource->SetVolume(0.2f);
			missSources[rand() % 3]->Play();
		};

	auto hitListener = [this](void* sender, void** args, uint32_t argc)
		{
			voicesSource->SetVolume(1.0f);
		};

	Stratum::EventHandler::RegisterListener(missListener, Stratum::EventHandler::GetEventID("miss_note"), true, true);
	Stratum::EventHandler::RegisterListener(hitListener, Stratum::EventHandler::GetEventID("hit_note"), true, true);

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
	instSource->Play();

	if (mConductor->chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), scene->AudioEngine->GetEngine());
		scene->AudioEngine->AddSource(voicesSource);
		voicesSource->Play();
	}

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

	startUpVideo = new SpriteVideo("fnf/videos/GatoBros.mp4", &mScene->Resources);
	startUpVideo->Create(mScene);

	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");

	static ECS::edict_t x64entity;
	static ECS::edict_t x64blackentity;

	mConductor->AddScriptedEvent(253, [this]() {
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
		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = true;
		sprite.RenderLayer = 102;
		sprite.IsGui = true;

		});

	mConductor->AddScriptedEvent(272, [this]() {
		mScene->EntityManager.DestroyEntity(x64blackentity);
		mScene->EntityManager.DestroyEntity(x64entity);
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
		});

	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});

	//instSource->Seek(105 * 44100);
	//voicesSource->Seek(105 * 44100);

	pEarlyUpdate = true;
}

void Funkin::InGameSystem::Update(Stratum::Scene* scene)
{
	if (mConductor->BeatCountF < 3.0f)
	{
		if (voicesSource)
			voicesSource->SetVolume(1.0f);
		instSource->SetVolume(1.0f);
	}

	startUpVideo->Update(scene);

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

	if ((mConductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat == 0)
	{
		float mult = 1.0f;

		if (gGameState.DoBeatEveryNthBeat == 2 && (mConductor->BeatCount + gGameState.BeatOffset) % 4 == 0)
		{
			mult = 1.8f;
		}

		if (!DoBeat)
		{
			GuiZoomLevel += 0.035f * mult;
			ZoomLevel += 0.025f * mult;
			DoBeat = true;
		}
	}

	if ((mConductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat != 0 && DoBeat)
		DoBeat = false;

	ZoomLevel = glm::mix(ZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);
	GuiZoomLevel = glm::mix(ZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);

	scene->RenderPath3D->RenderPath2D->SetGuiCameraZoom({ GuiZoomLevel, GuiZoomLevel });
	scene->RenderPath3D->RenderPath2D->SetCameraZoom({ ZoomLevel, ZoomLevel });

	scene->RenderPath3D->RenderPath2D->SetCameraPosition(gGameState.CameraPosition);

	UpdateStage();
}

void Funkin::InGameSystem::PostUpdate(Stratum::Scene* scene)
{
	CharaRegistry::Update();
}

void Funkin::InGameSystem::RenderImGui(Stratum::Scene* scene)
{
	return;
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");
	ImGui::Text("Beats: %i, Score: %i", mConductor->BeatCount, mConductor->PlayerScore);

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);
	ImGui::Text("Vram: %.2fmb", (float)Render::RendererContext::s_Context->GetGraphicsDeviceProperties().UsedVideoMemory / 1024.0f / 1024.0f);
	ImGui::Text("ECS Stats [Live/Max] %i/%i", mScene->EntityManager.LiveEntities, mScene->EntityManager.MaxEntities);

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();
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
	}
}

Funkin::CharaSprite* Funkin::InGameSystem::GetPlayerCharacter()
{
	return mPlayerCharacter;
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
			if (sprite.Rotation.x == 0.0f)
			{
				sprite.Rotation.x = rand() % 360;
			}
			sprite.Rotation.x += 360.0f * 1.5f * Stratum::gpGlobals->deltaTime;
		}

		transform.Position = glm::vec3(targetPos, 0.0f);

		transform.IsDirty = true;
	}
}
