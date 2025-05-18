#include "JavosApp.h"

#include "Input/Input.h"

#include "Sound/AudioSystem.h"
#include "Scene/Scene.h"

#include "Util/Globals.h"

#include <Thirdparty/imgui/imgui.h>
#include <Core/EngineStats.h>
#include <Core/Time.h>

#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"
#include "Player.h"

#undef min

Javos::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

Stratum::Ref<Stratum::MP3AudioSource> instSource;
Stratum::Ref<Stratum::MP3AudioSource> voicesSource;

Javos::Conductor* conductor;

void Javos::JavosApp::OnInit()
{
	auto gameScene = new Stratum::Scene();
	SetScene(gameScene);

	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);
	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteHoldComponent>(), C_NOTE_HOLD_COMPONENT_NAME);

	conductor = new Conductor();
	conductor->LoadChart(gameScene, "fnf/data/bite/bite-fernan.json");

	PlayerSystem* playerSystem;

	gameScene->RegisterCustomSystem(conductor);
	gameScene->RegisterCustomSystem(playerSystem = new PlayerSystem(conductor));

	std::string instPath = "fnf/songs/";
	std::string voicesPath = "fnf/songs/";
	instPath.append(conductor->chart.info.song).append("/Inst.mp3");
	voicesPath.append(conductor->chart.info.song).append("/Voices.mp3");

	playerSystem->CreatePlayer();

	instSource = Stratum::CreateRef<Stratum::MP3AudioSource>(instPath.c_str(), m_AudioEngine->GetEngine());
	m_AudioEngine->AddSource(instSource);
	instSource->Play();

	if (conductor->chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), m_AudioEngine->GetEngine());
		m_AudioEngine->AddSource(voicesSource);
		voicesSource->Play();
	}

	instSource->Seek(882000);
	voicesSource->Seek(882000);

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

	conductor->SongTime = instSource->PositionF();
}

void Javos::JavosApp::OnFrameRenderImGui()
{
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");
	ImGui::Text("Beats: %i", conductor->BeatCount);

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);
	ImGui::Text("Vram: %.2fmb", (float)Render::RendererContext::s_Context->GetGraphicsDeviceProperties().UsedVideoMemory / 1024.0f / 1024.0f);

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
