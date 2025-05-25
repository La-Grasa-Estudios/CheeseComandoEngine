#include "InGameSystem.h"

#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"
#include "Player.h"

#include "StageEditorSystem.h"

#include <Core/EngineStats.h>
#include <Core/Time.h>
#include <Core/Window.h>

#include <Util/Globals.h>
#include <Event/EventHandler.h>
#include <Input/Input.h>
#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>

#include <Thirdparty/imgui/imgui.h>
#include <json/json.hpp>


Javos::GameState gGameState;

#undef min
#undef max

struct StagePropComponent
{
	glm::vec2 Scroll;
	glm::vec2 Position;
	float DanceEvery;
};

Javos::InGameSystem::InGameSystem(const LoadChartParams& params) : mLoadParams(params)
{
	
}

Javos::InGameSystem::~InGameSystem()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
}

void Javos::InGameSystem::Init(Stratum::Scene* scene)
{
	gGameState = {};
	mScene = scene;

	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteHoldComponent>(), C_NOTE_HOLD_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<AnimatedEffectComponent>(), C_ANIMATED_EFFECT_COMPONENT_NAME);
	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<StagePropComponent>(), "stage_prop");

	mConductor = new Conductor();
	gGameState.pConductor = mConductor;

	PlayerSystem* playerSystem;

	mScene->RegisterCustomSystem(mConductor, true);
	mScene->RegisterCustomSystem(playerSystem = new PlayerSystem(mConductor, &gGameState), true);

	mConductor->LoadChart(mScene, mLoadParams.ChartPath);

	std::string instPath = C_SONG_PATH_PREFIX;
	std::string voicesPath = "fnf/songs/";
	instPath.append(mConductor->chart.info.song).append("/Inst.mp3");
	voicesPath.append(mConductor->chart.info.song).append("/Voices.mp3");

	mPlayerSprite = playerSystem->CreatePlayer();

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
			mFadeToWhiteIntensity = event.castFloat(event.Arg1);
			mFadeToWhiteBaseTime = event.castInteger(event.Arg2) / 1000.0f;
			mFadeToWhiteTime = 0.0f;
		});

	mWhiteSprite = mScene->EntityManager.CreateEntity();

	mScene->Transforms.Create(mWhiteSprite);

	auto& sprite = mScene->SpriteRenderers.Create(mWhiteSprite);
	sprite.Rect = { glm::ivec2(0, 0), glm::ivec2(10000, 10000) };
	sprite.IsGui = true;
	sprite.RenderLayer = 100;
	sprite.SpriteColor.a = 0.0f;

	LoadStage();

	instSource = Stratum::CreateRef<Stratum::MP3AudioSource>(instPath.c_str(), scene->AudioEngine->GetEngine());
	scene->AudioEngine->AddSource(instSource);
	instSource->Play();

	if (mConductor->chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), scene->AudioEngine->GetEngine());
		scene->AudioEngine->AddSource(voicesSource);
		voicesSource->Play();
	}
}

void Javos::InGameSystem::Update(Stratum::Scene* scene)
{
	if (Stratum::Input::GetKeyDown(KeyCode::F11))
	{
		static bool fs = false;
		scene->Window->SetFullScreen(fs = !fs);
	}

	if (Stratum::Input::GetKeyDown(KeyCode::NUMBER_7))
	{
		auto editor = new Stratum::Scene();

		editor->RegisterCustomSystem(new StageEditorSystem(""));

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
	whiteSprite.SpriteColor.a = glm::mix(0.0f, mFadeToWhiteIntensity, mFadeToWhiteTime / mFadeToWhiteBaseTime);

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

void Javos::InGameSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Javos::InGameSystem::RenderImGui(Stratum::Scene* scene)
{
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

void Javos::InGameSystem::LoadStage()
{
	std::string stagePath = GenerateAssetPath(C_STAGE_PATH_PREFIX, mConductor->chart.info.stage, "json");

	if (!Stratum::ZVFS::Exists(stagePath.c_str()))
		return;

	nlohmann::json json = nlohmann::json::parse(Stratum::ZVFS::GetFile(stagePath.c_str())->Str());
	auto metadataManager = mScene->GetComponentManager<StagePropComponent>("stage_prop");

	for (auto& prop : json["props"])
	{
		auto entity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& transform = mScene->Transforms.Create(entity);
		auto& name = mScene->Names.Create(entity);
		auto& meta = metadataManager->Create(entity);

		name.Name = prop["name"];
		sprite.TextureHandle = mScene->Resources.LoadTextureImage(prop["assetPath"]);
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.SpriteColor.a = prop["opacity"];
		sprite.RenderLayer = prop["zIndex"];
		meta.Position.x = prop["position"][0];
		meta.Position.y = prop["position"][1];
		meta.Scroll.x = prop["scroll"][0];
		meta.Scroll.y = prop["scroll"][1];
		transform.Scale.x = prop["scale"][0];
		transform.Scale.y = prop["scale"][1];
	}

	if (json.contains("characters"))
	{
		auto& characters = json["characters"];

		{
			auto& bf = characters["bf"];
			auto& transform = mScene->Transforms.Get(mPlayerSprite);
			auto& sprite = mScene->SpriteRenderers.Get(mPlayerSprite);

			sprite.RenderLayer = bf["zIndex"];
			gGameState.PlayerPosition.x = bf["position"][0];
			gGameState.PlayerPosition.y = bf["position"][1];
			transform.Scale.x = bf["scale"][0];
			transform.Scale.y = bf["scale"][1];
			CameraOffsets[0].x = bf["cameraOffset"][0];
			CameraOffsets[0].y = bf["cameraOffset"][1];
		}
	}
}

void Javos::InGameSystem::UpdateStage()
{
	auto metadataManager = mScene->GetComponentManager<StagePropComponent>("stage_prop");

	auto& props = metadataManager->GetEntities();

	for (auto entity : props)
	{
		auto& metadata = metadataManager->Get(entity);
		auto& transform = mScene->Transforms.Get(entity);

		glm::vec2 pos = metadata.Position;
		glm::vec2 scroll = metadata.Scroll;
		glm::vec2 targetPos = metadata.Position + gGameState.CameraPosition * scroll;

		transform.Position = glm::vec3(targetPos, 0.0f);

		transform.IsDirty = true;
	}
}
