#include "Application.h"

#include "Renderer/RendererContext.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/CopyCommandBuffer.h"
#include "Renderer/ShapeProvider.h"

#include "Scene/Scene.h"
#include "Scene/Renderer3D.h"
#include "Scene/Renderer2D.h"

#include <Entity/ClassMetadata.h>
#include <Entity/ComponentLibrary.h>
#include <AngelScript/AngelScript.h>

#include "VFS/ZVFS.h"
#include "Input/Input.h"
#include "Media/VideoDecode.h"
#include "JobManager.h"
#include "Time.h"
#include "Logger.h"
#include "VarRegistry.h"
#include "EngineStats.h"
#include "Event/EventBus.h"
#include "DevTools/ShaderCompiler.h"

#include "Util/PathUtils.h"
#include "Util/Globals.h"
#include "Util/CpuUtil.h"

#include "Thirdparty/imgui/imgui.h"

#include "Font/Font.h"

#include <filesystem>

using namespace ENGINE_NAMESPACE;

// Remnant of the old fury engine branch
ConsoleVar* g_GameRscDir;
Render::GraphicsPipeline* g_GraphicsPipeline;

std::binary_semaphore g_WaitForUpdate(0);
std::binary_semaphore g_WaitForRender(0);
std::binary_semaphore g_WaitForUpdateFinish(0);

std::atomic_bool g_FinishUpdateThread;

extern size_t totalAllocated;
extern size_t totalAllocations;
extern size_t freedAllocations;

extern void resetTotalAllocs();

Ref<Render::RendererContext> g_RenderContext;

Application::Application(ApplicationInfo& appInfo)
{
	this->m_AppInfo = appInfo;
	this->m_Window = NULL;
	s_Instance = this;
}

void Application::Run(std::vector<std::string> args)
{
	gpGlobals = new GlobalVars();
	{
		volatile auto utilInit = Utils::CpuUtil();
	}

	// I know how to update the build date every time it compiles
	// BUT i don't like the fact that every debug session i need to recompile a single file
	// Pls microsoft, add a feature so i can run a command when there is a recompilation happening

	Z_INFO("Stratum Engine {} @ {} ", __DATE__, __TIME__);

	ZVFS::Init();
	JobManager::Init(true);

	// Idk why the engine crashes with a segfault without this hack
	m_Console.Focused();
	
	ZVFS::Mount("Pak");
	//ZVFS::Mount("Data", true);

	VarRegistry::RegisterConsoleVar("", "quit", VarType::Void)->func = [&](ConsoleVar& var, std::string args) {
		EventBus::InvokeEvent(ApplicationEvent{ ApplicationEvent::APP_EVENT_SHUTDOWN });
	};

	VarRegistry::RegisterConsoleVar("cl", "imgui", VarType::Bool)->set(m_AppInfo.IsImGuiEnabled);
	VarRegistry::RegisterConsoleVar("cl", "api", VarType::Int)->set((int)m_AppInfo.graphicsAPI);
	VarRegistry::RegisterConsoleVar("r", "vsync", VarType::Bool)->set(m_AppInfo.VSyncEnabled)->setOnModifyCallback(
		[this](ConsoleVar& var)
		{
			m_Window->SetVSync(var.asBool());
		}
	);
	VarRegistry::RegisterConsoleVar("cl", "appname", VarType::String)->set(m_AppInfo.WindowName)->setOnModifyCallback(
		[this](ConsoleVar& var)
		{
			m_Window->SetName(var.data);;
		}
	);
	VarRegistry::RegisterConsoleVar("app", "compile", VarType::Void)->func = [&](ConsoleVar& var, std::string args) {

		if (args.empty()) {
			Z_INFO("Usage: app_compile <path_to_glsl_file> <path_to_spirv_output> <shader type (frag/vert/geom/comp)>");
			return;
		}

		std::size_t outoffset = args.find_first_of(" ");

		if (outoffset == std::string::npos) {
			Z_WARN("Invalid app_compile args: {}", args);
			return;
		}

		std::size_t typeoffset = args.find_first_of(" ", outoffset + 1);
		std::size_t perms = args.find_first_of(" ", typeoffset + 1);

		if (outoffset == std::string::npos) {
			Z_WARN("Invalid app_compile args: {}", args);
			return;
		}

		std::string shaderPath;
		shaderPath.resize(outoffset);
		std::string shaderOut;
		shaderOut.resize(typeoffset - outoffset);
		std::string shaderType;
		shaderType.resize(4);
		std::string shaderPerms;
		shaderPerms.resize(1);

		memcpy(shaderPath.data(), args.data(), outoffset);
		memcpy(shaderOut.data(), args.data() + outoffset + 1, typeoffset - outoffset);
		memcpy(shaderType.data(), args.data() + typeoffset + 1, 4);

		if (perms != std::string::npos) {
			memcpy(shaderPerms.data(), args.data() + perms + 1, 1);
		}
		else {
			shaderPerms = "1";
		}

		ShaderCompiler::shader_type type = ShaderCompiler::shader_type::fragment;

		if (shaderType.compare("vert") == 0) {
			type = ShaderCompiler::shader_type::vertex;
		}

		if (shaderType.compare("geom") == 0) {
			type = ShaderCompiler::shader_type::geometry;
		}

		if (shaderType.compare("comp") == 0) {
			type = ShaderCompiler::shader_type::compute;
		}

		int nbPerms = std::stoi(shaderPerms);

		ShaderCompiler::build_object(shaderPath.c_str(), shaderOut.c_str(), type, nbPerms);

		};
	g_GameRscDir = VarRegistry::RegisterConsoleVar("cl", "rsc_dir", VarType::String)->set("");
	g_GameRscDir->setOnModifyCallback(
		[this](ConsoleVar& var)
		{
			ZVFS::Mount(var.str(), true);
		}
	);

	VarRegistry::RegisterConsoleVar("", "exec", VarType::Void)->func = [&](ConsoleVar& var, std::string args) {

		if (args.empty()) return;

		std::string path = std::string(*g_GameRscDir).append("/config/");

		if (args.find("game") != std::string::npos) {
			path = "";
		}

		path.append(args).append(".cfg");

		VarRegistry::RunCfg(path);

	};
	VarRegistry::RegisterConsoleVar("", "say", VarType::Void)->func = [&](ConsoleVar& var, std::string args) {

		if (args.empty()) return;

		Z_INFO(args);

	};
	
	std::string autoExecLog;
	VarRegistry::ParseConsoleVar("exec Engine/game", autoExecLog);

	bool SingleThreaded = false;
	bool EnableVids = true;

	for (int i = 0; i < args.size(); i++)
	{
		std::string str = args[i];

		if (str == "-vfs-verbose")
		{
			ZVFS::g_VFSDebug = true;
		}

		if (str == "-singlethread")
		{
			SingleThreaded = true; // This poor guy is ignored lol
		}

		if (str.starts_with("-adapter="))
		{
			std::string adapterId = str.substr(9);
			Z_INFO("Using adapter: {}", adapterId);
			VarRegistry::RegisterConsoleVar("r", "adapter", VarType::Int);
			VarRegistry::ParseConsoleVar(std::string("r_adapter ").append(adapterId), autoExecLog);
		}

		if (str == "-novid")
		{
			EnableVids = false;
		}
	}

	m_RenderContext = CreateRef<Render::RendererContext>();
	g_RenderContext = m_RenderContext;


	OnEarlyInit(); // Use this to hook into events and stuff
	SceneUI::EarlyInit();

	AngelScriptEngine::Get().Init();

	std::string windowName = m_AppInfo.WindowName;

	m_Window = CreateScope<Internal::Window>(m_RenderContext.get(), windowName.c_str());
	m_AudioEngine = CreateRef<AudioEngine>();

	m_Window->SetInfo(Internal::WindowEnum::WINDOW_START_MAXIMIZED, m_AppInfo.ShouldWindowStartMaximized);
	m_Window->SetInfo(Internal::WindowEnum::WINDOW_IMGUI, m_AppInfo.IsImGuiEnabled);
	m_Window->SetInfo(Internal::WindowEnum::WINDOW_FULLSCREEN, m_AppInfo.Fullscreen);
	m_Window->SetVSync(m_AppInfo.VSyncEnabled);

	m_RenderContext->InitializeApi(m_AppInfo.graphicsAPI);
	m_Window->Create(m_AppInfo.WindowedResolutionX, m_AppInfo.WindowedResolutionY);

	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_WINDOW));

	if (m_AppInfo.IsImGuiEnabled) {

		IMGUI_CHECKVERSION();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		m_RenderContext->ImGuiInit(m_Window.get());

	}

	m_AudioEngine->Init();
	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_AUDIO));

	Input::Init(m_Window->GetHandle());
	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_INPUT));

	ComponentMetadata::Init();

	// Remove this on retail builds?
	ShaderCompiler::build_object("shaders/video_gbar_to_rgba.hlsl", "Data/shaders/video_gbar_to_rgba.cso", ShaderCompiler::shader_type::vertex, 1);
	ShaderCompiler::build_object("shaders/deferred_gbuffer_opaque.hlsl", "Data/shaders/deferred_gbuffer_opaque.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/2d/2d_sprite.hlsl", "Data/shaders/2d/2d_sprite.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/tone_map.hlsl", "Data/shaders/tone_map.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/post/bloom_downsample.hlsl", "Data/shaders/post/bloom_downsample.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/post/bloom_upsample.hlsl", "Data/shaders/post/bloom_upsample.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/compute/compute_bloom_filter.hlsl", "Data/shaders/compute/compute_bloom_filter.cso", ShaderCompiler::shader_type::compute);
	ShaderCompiler::build_object("shaders/compute/compute_luminance.hlsl", "Data/shaders/compute/compute_luminance.cso", ShaderCompiler::shader_type::compute);
	ShaderCompiler::build_object("shaders/compute/compute_avg_luminance.hlsl", "Data/shaders/compute/compute_avg_luminance.cso", ShaderCompiler::shader_type::compute);
	ShaderCompiler::build_object("shaders/compute/compute_chromatic_aberration.hlsl", "Data/shaders/compute/compute_chromatic_aberration.cso", ShaderCompiler::shader_type::compute);

	Render::GraphicsDeviceProperties gdprop = m_RenderContext->GetGraphicsDeviceProperties();

	// $h1t i need to account for the swapchain in the vram usage, currently reports 0mb at startup 
	// Only affects texture streaming but the difference is about 10mb so not high priority lol
	Z_INFO("{}, VRAM: {} MB, SHARED: {} MB, USED: {} MB", gdprop.Description,
		gdprop.DedicatedVideoMemory / 1024 / 1024,
		gdprop.SharedVideoMemory / 1024 / 1024,
		gdprop.UsedVideoMemory / 1024 / 1024);

	// Inspired by source engine :)
	RenderStartupMedia();

	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_LATE_INIT));

	m_RenderPath3D = CreateRef<Renderer3D>();
	m_RenderPath2D = CreateRef<Renderer2D>();

	if (!m_Window->CloseRequested())
	{
		VarRegistry::ParseConsoleVar("exec startup", autoExecLog);

		OnInit();

		VarRegistry::ParseConsoleVar("exec autoexec", autoExecLog);
		if (!autoExecLog.empty()) {
			Z_ERROR("Failed to run autoexec: {}", autoExecLog);
		}
	}

	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_POST_INIT));

	MainLoop();

	// Still here bc of an old fury branch where i had a dedicated render thread
	// Now multithreading is implemented like id tech 7 (A lot of jobs)
	g_FinishUpdateThread.store(true);

	Render::ShapeProvider::Release();

	if (m_AppInfo.IsImGuiEnabled) {
		m_RenderContext->ImGuiShutdown();
	}

	m_AudioEngine->Shutdown();
	VarRegistry::Cleanup();

	m_Window->Destroy();

	m_RenderContext = NULL; // RHI segfaults if i call release on this, idc since we are closing anyway, just a different way of doing that
}

void Application::MainLoop()
{

	m_Window->SetVSync(m_AppInfo.VSyncEnabled);

	bool LogStutters = false;
	float LastFrameDelta = 0.0f;
	float AllocTimer = 0.0f;

	bool ShouldClose = false;

	ComponentLibrary library{};

	EventBus::RegisterListener<ApplicationEvent>([&](const ApplicationEvent& e)
		{
			if (e.Type == e.APP_EVENT_SHUTDOWN)
				ShouldClose = true;
		}, EF_NONE);

	while (1) {

		Z_PROFILE_SCOPE("Application::Frame");

		if (m_Window->CloseRequested() || ShouldClose) {
			break;
		}

		constexpr float MaxStutterTime = 33.0f;
		float FrameDeltaDiff = (Time::DeltaTime - LastFrameDelta) * 1000.0f;
		LastFrameDelta = Time::DeltaTime;

		LogStutters = true;

		Time::BeginProfile();
		Time::ClearGPU();

#ifndef TRACY_ENABLE
		if (!m_Window->IsWindowActive()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			LogStutters = false;
		}
#endif

		EngineStats::Reset();

		Time::BeginCPU();

		m_RenderContext->BeginFrame();

		JobManager::ExecuteMainJobs();

		gpGlobals->deltaTime = Time::DeltaTime;
		gpGlobals->elapsedTime = Time::GlobalTime;
		InternalUpdate();

		m_RenderPath2D->UpdateScreenSize(m_Window->GetFramebuffer()->GetSize());

		if (mCurrentScene)
		{
			Z_PROFILE_SCOPE("Scene::Update");

			if (mCurrentScene->NextScenePtr)
			{
				Stratum::EventBus::RemoveSceneEventListeners();
				SetScene(mCurrentScene->NextScenePtr);
			}

			mCurrentScene->VirtualScreenSize = m_RenderPath2D->VirtualScreenSize;
			mCurrentScene->UpdateSystems();
		}

		{
			Z_PROFILE_SCOPE("Application::FrameUpdate");
			OnFrameUpdate();
		}

		if (mCurrentScene)
		{
			Z_PROFILE_SCOPE("Scene::PostUpdate");
			mCurrentScene->PostUpdate();
		}

		Input::Update();

		OnFramePrepare();

		OnFrameRender();

		glm::mat4 projection = glm::perspective(glm::radians(70.0f), m_Window->GetWidth() / (float)m_Window->GetHeight(), 0.01f, 100.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		m_RenderPath3D->SetViewPose(ViewPose(projection, view));

		if (mCurrentScene)
		{

			m_RenderPath3D->PreRender(mCurrentScene, m_Window->GetFramebuffer().get());
			m_RenderPath2D->PreRender(mCurrentScene, m_Window->GetFramebuffer().get());

			m_RenderPath3D->Render(mCurrentScene, m_Window->GetFramebuffer().get());
			m_RenderPath2D->Render(mCurrentScene, m_Window->GetFramebuffer().get());

			m_RenderPath2D->Submit();
		}

		if (m_AppInfo.IsImGuiEnabled)
		{

			m_RenderContext->ImGuiBeginFrame();

			ImGui::NewFrame();

			OnFrameRenderImGui();

			if (mCurrentScene)
			{
				mCurrentScene->RenderImGui();
			}

			ImGui::Render();

			m_RenderContext->ImGuiEndFrame();

		}

		m_Window->Update();

		Time::EndCPU();

		Time::EndProfile();

		if (LogStutters)
		{
			float total = 0.0f;
			auto stats = EngineStats::GetTimes();

			for (int i = 0; i < stats.size(); i++)
			{
				ScopedTime& time = stats[i];
				total += time.time;
			}

			if (total > 30000003.0f)
			{
				Z_WARN("Frame got {:.3f}ms over 33ms max!", total);
				Z_WARN("Frame scope");

				auto stats = EngineStats::GetTimes();

				for (int i = 0; i < stats.size(); i++)
				{
					ScopedTime& time = stats[i];
					Z_INFO("{}: {:.3f}ms", time.name, time.time);
				}

				if (stats.empty())
				{
					Z_WARN("Timing stats are empty! maybe this is the first frame?");
				}
			}
			
		}
		LogStutters = false;

		AllocTimer += Time::UnscaledDeltaTime;

		if (AllocTimer > 1.0f)
		{
			AllocTimer = 0.0f;

			gpGlobals->totalAllocated = totalAllocated;
			gpGlobals->freedAllocations = freedAllocations;
			gpGlobals->totalAllocations = totalAllocations;

			resetTotalAllocs();
		}
	}

	Cleanup();
	SetScene(NULL);

	m_RenderPath3D = NULL;
	m_RenderPath2D = NULL;
}

void Application::RenderStartupMedia()
{
	EventBus::InvokeEvent(EngineModuleInitEvent(EngineModuleInitEvent::ENGINE_MODULE_MEDIA));

	if (ZVFS::Exists("media/startupvids.txt"))
	{

		std::string path = "media/startupvids.txt";

		RefBinaryStream vids = ZVFS::GetFile(path.c_str());

		std::string line;

		Render::PipelineDescription pipelineDesc;

		pipelineDesc.ShaderPath = "shaders/video_gbar_to_rgba.cso";

		pipelineDesc.RenderTarget = m_Window->GetFramebuffer().get();

		pipelineDesc.RasterizerState.DepthTest = false;

		pipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH16;

		Render::GraphicsPipeline videoPipeline(pipelineDesc);

		Render::GraphicsCommandBuffer cmdBuffer{};
		Render::CopyCommandBuffer copyCmdBuffer{};

		bool firstVideo = true;

		while (std::getline(*vids->Stream(), line))
		{
			if (firstVideo && m_AppInfo.graphicsAPI == Render::RendererAPI::VULKAN)
			{
				line = "media/vulkan.mp4";
				vids = ZVFS::GetFile(path.c_str());
				firstVideo = false;
			}

			std::string vpath = PathUtils::ResolvePath(line);

			VideoDecode decode(vpath, m_AudioEngine.get());
			VideoParams params = decode.GetSize();

			if (params.width == 0) continue;

			int size = params.width * params.height * 4;

			Render::ImageDescription desc;
			desc.Width = params.width;
			desc.Height = params.height;
			desc.Format = Render::ImageFormat::RGBA8_UNORM;

			Ref<Render::ImageResource> surface = CreateRef<Render::ImageResource>(desc);
			Render::TextureSamplerDescription samplerDesc{};
			Render::TextureSampler sampler(samplerDesc);

			float frameTime = decode.GetFrametime();
			float accum = 0.0f;

			float globalTime = 0;
			float sleepTime = globalTime;
			int fIndex = 0;

			int frames = 0;

			bool firstFrameReady = false;

			bool end = false;

			bool frameIndex = 0;

			while (!decode.Finished() && !m_Window->CloseRequested())
			{
				m_RenderContext->BeginFrame();

				m_Window->SetVSync(false);

				frameIndex = !frameIndex;

				Time::BeginProfile();

				decode.Step();

				JobManager::ExecuteMainJobs();

				accum += Time::DeltaTime;

				while (accum >= frameTime && !decode.Finished())
				{
					auto frame = decode.GetFrame();
					if (frame)
					{
						auto cmd = copyCmdBuffer.GetNativeCommandList();

						copyCmdBuffer.Begin();
						copyCmdBuffer.RequireTextureState(surface.get(), nvrhi::ResourceStates::ShaderResource, nvrhi::ResourceStates::CopyDest);

						cmd->writeTexture(surface->Handle, 0, 0, frame->native()->data[0], params.width * 4);

						copyCmdBuffer.RequireTextureState(surface.get(), nvrhi::ResourceStates::CopyDest, nvrhi::ResourceStates::ShaderResource);
						copyCmdBuffer.End();

						copyCmdBuffer.WaitForExecution(cmdBuffer.GetQueueExecutionInstance(), Render::CommandQueue::Graphics);
						copyCmdBuffer.Submit();

						cmdBuffer.WaitForExecution(copyCmdBuffer.GetQueueExecutionInstance(), Render::CommandQueue::Copy);

						firstFrameReady = true;

						decode.PushFrame(frame);
						fIndex++;
					}
					accum -= frameTime;
				}

				if (!decode.Finished()) decode.StepAudio(globalTime);

				Render::Viewport vp{};
				vp.width = m_Window->GetWidth();
				vp.height = m_Window->GetHeight();

				cmdBuffer.Begin();

				cmdBuffer.RequireFramebufferState(m_Window->GetFramebuffer().get(), Render::ResourceState::Present, Render::ResourceState::RenderTarget);
				cmdBuffer.SetViewport(&vp);
				cmdBuffer.SetPipeline(&videoPipeline);
				cmdBuffer.SetFramebuffer(m_Window->GetFramebuffer().get());
				cmdBuffer.SetTextureSampler(&sampler, 0);
				cmdBuffer.SetTextureResource(surface.get(), 0);

				cmdBuffer.Draw(3, 0);

				cmdBuffer.RequireFramebufferState(m_Window->GetFramebuffer().get(), Render::ResourceState::RenderTarget, Render::ResourceState::Present);

				cmdBuffer.End();

				cmdBuffer.Submit();

				m_Window->Update();
				Input::Update();

				JobManager::Wait();

				globalTime += Time::DeltaTime;
				sleepTime += frameTime;

				float diff = sleepTime - globalTime;

				if (diff > 0.0f)
				{
					int nano = diff * 1000 * 1000 * 1000;
					std::this_thread::sleep_for(std::chrono::nanoseconds(nano));
				}

				Time::EndProfile();

				if (Input::AnyKeyDown())
				{
					break;
				}

			}
		}


	}
	else
	{
		Z_WARN("Failed to open file media/startupvids.txt");
	}
}
void Application::InternalUpdate()
{
	// TO DO: Move this to main loop
	Z_PROFILE_SCOPE("Application::InternalUpdate");
	gpGlobals->gametic++;
}

void Application::SetScene(Scene* scene)
{
	if (mCurrentScene)
	{
		JobManager::Wait();
		Render::RendererContext::GetDevice()->waitForIdle();
		delete mCurrentScene;
	}

	mCurrentScene = scene;
	gpGlobals->gametic = 0;

	// If this pointer has been asigned scene has been probably initialized somewhere else (Async scene loading)
	if (scene && !scene->RenderPath3D)
		InitSceneResources(scene);

	if (scene)
		m_RenderPath2D->UpdateScreenSize(m_Window->GetFramebuffer()->GetSize());

	Time::TimeScale = 1.0f;
}

void Application::InitSceneResources(Scene* scene)
{
	if (scene)
	{
		m_RenderPath3D->SetScene(scene);
		scene->AudioEngine = m_AudioEngine.get();
		scene->Window = m_Window.get();
		scene->RenderPath3D = m_RenderPath3D.get();
		scene->RenderPath2D = m_RenderPath2D.get();
	}
}

void Application::OnEarlyInit()
{
}

void Application::OnInit()
{
	
}

void Application::OnFrameUpdate()
{
	
}

void Application::OnFrameSync()
{
}

void Application::OnFramePrepare()
{
}

void Application::OnFrameRender()
{
}

void Application::On2DRender()
{
}

#undef min

void Application::OnFrameRenderImGui()
{
	return;
	static int frameRate = 0;
	
	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	m_Console.Draw();

	static float frametimePlot[256] = {};
	static float gputimePlot[256] = {};

	for (int i = 255; i > 0; i--)
	{
		frametimePlot[i] = frametimePlot[i - 1];
	}

	frametimePlot[0] = gpGlobals->deltaTime * 1000.0f;

	for (int i = 255; i > 0; i--)
	{
		gputimePlot[i] = gputimePlot[i - 1];
	}

	gputimePlot[0] = Time::GPUTime.load() * 1000.0f;

	ImGui::Begin("EngineStats");

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);

	ImGui::PlotLines("##frametime", frametimePlot, IM_ARRAYSIZE(frametimePlot), 0, NULL, 0.0f, 50.0f, ImVec2(0, 80));
	ImGui::PlotLines("##gputime", gputimePlot, IM_ARRAYSIZE(gputimePlot), 0, NULL, 0.0f, 50.0f, ImVec2(0, 80));

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();
}

void Application::Cleanup()
{
}
