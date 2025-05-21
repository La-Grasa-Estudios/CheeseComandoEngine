#include "JavosApp.h"

#include "Input/Input.h"

#include "Sound/AudioSystem.h"

#include "Util/Globals.h"

#include <Thirdparty/imgui/imgui.h>

#include <Core/EngineStats.h>
#include <Core/Time.h>
#include <Event/EventHandler.h>
#include <Scene/Scene.h>
#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>

#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"
#include "Player.h"

#undef min
#undef max

Javos::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

Stratum::Ref<Stratum::MP3AudioSource> instSource;
Stratum::Ref<Stratum::MP3AudioSource> voicesSource;

Stratum::Ref<Stratum::MP3AudioSource> missSources[3];

Javos::Conductor* conductor;
Javos::GameState gGameState;

Stratum::ECS::edict_t gWhiteSprite;
float gFadeToWhiteTime = 0.0f;
float gFadeToWhiteBaseTime = 0.0f;
float gFadeToWhiteIntensity = 0.0f;

void Javos::JavosApp::OnInit()
{

	gGameState = {};

	auto gameScene = new Stratum::Scene();
	SetScene(gameScene);

	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);
	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteHoldComponent>(), C_NOTE_HOLD_COMPONENT_NAME);
	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<AnimatedEffectComponent>(), C_ANIMATED_EFFECT_COMPONENT_NAME);

	conductor = new Conductor();

	PlayerSystem* playerSystem;

	gameScene->RegisterCustomSystem(conductor);
	gameScene->RegisterCustomSystem(playerSystem = new PlayerSystem(conductor, &gGameState));

	conductor->LoadChart(gameScene, "fnf/data/bite/bite-fernan.json");

	std::string instPath = "fnf/songs/";
	std::string voicesPath = "fnf/songs/";
	instPath.append(conductor->chart.info.song).append("/Inst.mp3");
	voicesPath.append(conductor->chart.info.song).append("/Voices.mp3");

	playerSystem->CreatePlayer();

	for (int i = 0; i < 3; i++)
	{
		std::string path = "fnf/sounds/missnote";
		path.append(std::to_string(i + 1)).append(".mp3");
		missSources[i] = Stratum::CreateRef<Stratum::MP3AudioSource>(path.c_str(), m_AudioEngine->GetEngine());
		missSources[i]->SetVolume(0.25f);
		m_AudioEngine->AddSource(missSources[i]);
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

	Stratum::EventHandler::RegisterListener(missListener, Stratum::EventHandler::GetEventID("miss_note"), true);
	Stratum::EventHandler::RegisterListener(hitListener, Stratum::EventHandler::GetEventID("hit_note"), true);

	conductor->RegisterEventHandler("StSetScreenBeat", [this](ChartEvent& event)
		{
			gGameState.DoBeatEveryNthBeat = event.castInteger(event.Arg1);
			
			int offset = event.castInteger(event.Arg2);
			
			if (offset != 0)
				gGameState.BeatOffset = offset;
		});

	conductor->RegisterEventHandler("StFadeToWhite", [this](ChartEvent& event)
		{
			gFadeToWhiteIntensity = event.castFloat(event.Arg1);
			gFadeToWhiteBaseTime = event.castInteger(event.Arg2) / 1000.0f;
			gFadeToWhiteTime = 0.0f;
		});

	instSource = Stratum::CreateRef<Stratum::MP3AudioSource>(instPath.c_str(), m_AudioEngine->GetEngine());
	m_AudioEngine->AddSource(instSource);
	instSource->Play();

	if (conductor->chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), m_AudioEngine->GetEngine());
		m_AudioEngine->AddSource(voicesSource);
		voicesSource->Play();
	}

	//instSource->Seek(105 * 44100);
	//voicesSource->Seek(105 * 44100);

	gWhiteSprite = gameScene->EntityManager.CreateEntity();

	gameScene->Transforms.Create(gWhiteSprite);

	auto& sprite = gameScene->SpriteRenderers.Create(gWhiteSprite);
	sprite.Rect = { glm::ivec2(0, 0), glm::ivec2(10000, 10000) };
	sprite.IsGui = true;
	sprite.RenderLayer = sprite.LAYER_FG2;
	sprite.SpriteColor.a = 0.0f;

}

void Javos::JavosApp::OnFrameUpdate()
{

	if (Stratum::Input::GetKeyDown(KeyCode::F11))
	{
		static bool fs = false;
		m_Window->SetFullScreen(fs = !fs);
	}

	float bpmPerSecond = 1.0f / (conductor->chart.info.bpm / 60.0f);
	static float songDeltaTime = 0.0f;
	static float lastSongTime = 0.0f;
	static glm::vec3 bfPosition = glm::vec3(0.0f);

	static float GuiZoomLevel = 1.0f;
	static float ZoomLevel = 1.0f;
	static bool DoBeat = false;

	conductor->SongTime = instSource->PositionF();

	gFadeToWhiteBaseTime = glm::max(gFadeToWhiteBaseTime, 0.001f);

	gFadeToWhiteTime += Stratum::gpGlobals->deltaTime;
	gFadeToWhiteTime = glm::min(gFadeToWhiteTime, gFadeToWhiteBaseTime);

	auto& whiteSprite = mCurrentScene->SpriteRenderers.Get(gWhiteSprite);
	whiteSprite.SpriteColor.a = glm::mix(0.0f, gFadeToWhiteIntensity, gFadeToWhiteTime / gFadeToWhiteBaseTime);

	if ((conductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat == 0)
	{
		float mult = 1.0f;

		if (gGameState.DoBeatEveryNthBeat == 2 && (conductor->BeatCount + gGameState.BeatOffset) % 4 == 0)
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

	if ((conductor->BeatCount + gGameState.BeatOffset) % gGameState.DoBeatEveryNthBeat != 0 && DoBeat)
		DoBeat = false;

	ZoomLevel = glm::mix(ZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);
	GuiZoomLevel = glm::mix(ZoomLevel, 1.0f, 5.0f * Stratum::gpGlobals->deltaTime);

	m_RenderPath3D->RenderPath2D->SetGuiCameraZoom({ GuiZoomLevel, GuiZoomLevel });
	m_RenderPath3D->RenderPath2D->SetCameraZoom({ ZoomLevel, ZoomLevel });

}

void Javos::JavosApp::OnFrameRenderImGui()
{
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");
	ImGui::Text("Beats: %i, Score: %i", conductor->BeatCount, conductor->PlayerScore);

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);
	ImGui::Text("Vram: %.2fmb", (float)Render::RendererContext::s_Context->GetGraphicsDeviceProperties().UsedVideoMemory / 1024.0f / 1024.0f);
	ImGui::Text("ECS Stats [Live/Max] %i/%i", mCurrentScene->EntityManager.LiveEntities, mCurrentScene->EntityManager.MaxEntities);

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();
}

void Javos::JavosApp::Cleanup()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
}
