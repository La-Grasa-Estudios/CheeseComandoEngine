#include "Application.h"

#define OPENGL_RENDERER

#include "Renderer/RendererContext.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ShapeProvider.h"

#include "Scene/Scene.h"
#include "Scene/Renderer3D.h"

#include "VFS/ZVFS.h"
#include "Input/Input.h"
#include "Media/VideoDecode.h"
#include "JobManager.h"
#include "Time.h"
#include "Logger.h"
#include "VarRegistry.h"
#include "EngineStats.h"
#include "Event/EventHandler.h"
#include "DevTools/ShaderCompiler.h"

#include "Util/PathUtils.h"
#include "Util/Globals.h"

#include "Thirdparty/imgui/imgui.h"

#include <filesystem>

using namespace ENGINE_NAMESPACE;

ConsoleVar* g_GameRscDir;
Render::GraphicsPipeline* g_GraphicsPipeline;

std::binary_semaphore g_WaitForUpdate(0);
std::binary_semaphore g_WaitForRender(0);
std::binary_semaphore g_WaitForUpdateFinish(0);

std::atomic_bool g_FinishUpdateThread;

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
#ifdef _DEBUG
	Z_INFO("Stratum Engine {} @ {} ", __DATE__, __TIME__);
#endif

	ZVFS::Init();

	m_Console.Focused();
	
	ZVFS::Mount("Pak");
	ZVFS::Mount("Data", true);

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

	for (int i = 0; i < args.size(); i++)
	{
		std::string str = args[i];

		if (str == "-vfs-verbose")
		{
			ZVFS::g_VFSDebug = true;
		}

		if (str == "-singlethread")
		{
			SingleThreaded = true;
		}

	}

	m_RenderContext = CreateRef<Render::RendererContext>();

	g_RenderContext = m_RenderContext;

	OnEarlyInit(); // Use this to hook into events and stuff

	std::string windowName = m_AppInfo.WindowName;

	m_RenderContext->InitializeApi(Render::RendererAPI::DX12);

	m_Window = CreateScope<Internal::Window>(m_RenderContext.get(), windowName.c_str());
	m_Window->SetInfo(Internal::WindowEnum::WINDOW_START_MAXIMIZED, m_AppInfo.ShouldWindowStartMaximized);
	m_Window->SetInfo(Internal::WindowEnum::WINDOW_IMGUI, m_AppInfo.IsImGuiEnabled);
	m_Window->SetInfo(Internal::WindowEnum::WINDOW_FULLSCREEN, m_AppInfo.Fullscreen);
	m_Window->SetVSync(m_AppInfo.VSyncEnabled);

	m_Window->Create(m_AppInfo.WindowedResolutionX, m_AppInfo.WindowedResolutionY);

	//JobManager::Init(!m_RenderContext->IsCapabilitySupported(Render::RendererCapability::RENDERER_MULTITHREADED_CMD_LISTS) || SingleThreaded);
	JobManager::Init(true);

	EventHandler::InvokeEvent(EventHandler::GetEventID("init_window"), this);

	Render::GraphicsDeviceProperties gdprop = m_RenderContext->GetGraphicsDeviceProperties();

	if (gdprop.DedicatedVideoMemory / 1024.0f / 1024.0f < 800)
	{
		//Platform::ShowSystemMessageBox("Insufficient Video Memory", "Your system has less than 1GB of video ram, glitches may occur!", Platform::MessageBoxButtons::OK, Platform::MessageBoxIcons::Warning);
	}

	if (m_AppInfo.IsImGuiEnabled) {

		IMGUI_CHECKVERSION();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		m_RenderContext->ImGuiInit(m_Window.get());

	}

	m_AudioEngine = CreateRef<AudioEngine>();
	m_AudioEngine->Init();

	EventHandler::InvokeEvent(EventHandler::GetEventID("init_modules"), this);

	Input::Init(m_Window->GetHandle());

	EventHandler::InvokeEvent(EventHandler::GetEventID("init_input"), this);

	ShaderCompiler::build_object("shaders/video_gbar_to_rgba.hlsl", "Data/shaders/video_gbar_to_rgba.cso", ShaderCompiler::shader_type::vertex, 1);
	ShaderCompiler::build_object("shaders/deferred_gbuffer_opaque.hlsl", "Data/shaders/deferred_gbuffer_opaque.cso", ShaderCompiler::shader_type::vertex);

	Z_INFO("Printing cmdline args");

	bool EnableVids = true;

	for (int i = 0; i < args.size(); i++)
	{
		std::string str = args[i];

		Z_INFO("[{}] {}", i, str);

		if (str == "-novid")
		{
			EnableVids = false;
		}

	}

	Z_INFO("{}, VRAM: {} MB, SHARED: {} MB, USED: {} MB", gdprop.Description,
		gdprop.DedicatedVideoMemory / 1024 / 1024,
		gdprop.SharedVideoMemory / 1024 / 1024,
		gdprop.UsedVideoMemory / 1024 / 1024);

	RenderStartupMedia();

	EventHandler::InvokeEvent(EventHandler::GetEventID("late_init"), this);

	if (!m_Window->CloseRequested())
	{

		VarRegistry::ParseConsoleVar("exec startup", autoExecLog);

		OnInit();

		VarRegistry::ParseConsoleVar("exec autoexec", autoExecLog);
		if (!autoExecLog.empty()) {
			Z_ERROR("Failed to run autoexec: {}", autoExecLog);
		}
	}

	EventHandler::InvokeEvent(EventHandler::GetEventID("post_init"), this);

	m_Window->SetVSync(false);

	MainLoop();

	g_FinishUpdateThread.store(true);

	Render::ShapeProvider::Release();

	if (m_AppInfo.IsImGuiEnabled) {
		m_RenderContext->ImGuiShutdown();
	}

	m_AudioEngine->Shutdown();
	VarRegistry::Cleanup();

	m_Window->Destroy();

	m_RenderContext = NULL;
}

void Application::MainLoop()
{
	m_Window->SetVSync(true);

	bool LogStutters = false;
	float LastFrameDelta = 0.0f;

	Scene* scene = new Scene();;
	Renderer3D rendererPath3D;

	rendererPath3D.SetScene(scene);

	scene->LoadModel("binbows.mdl", scene->EntityManager.CreateEntity());

	while (1) {

		Z_PROFILE_SCOPE("Application::Frame");

		if (m_Window->CloseRequested()) {
			break;
		}

		constexpr float MaxStutterTime = 33.0f;
		float FrameDeltaDiff = (Time::DeltaTime - LastFrameDelta) * 1000.0f;
		LastFrameDelta = Time::DeltaTime;

		if (FrameDeltaDiff > MaxStutterTime)
		{
			LogStutters = true;
		}

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

		JobManager::ExecuteMainJobs();
		EventHandler::Process();

		gpGlobals->deltaTime = Time::DeltaTime;
		InternalUpdate();

		OnFramePrepare();

		OnFrameRender();

		glm::mat4 projection = glm::perspective(glm::radians(70.0f), m_Window->GetWidth() / (float)m_Window->GetHeight(), 0.01f, 100.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		rendererPath3D.SetViewPose(ViewPose(projection, view));

		rendererPath3D.PreRender(scene);

		rendererPath3D.Render(scene, m_Window->GetFramebuffer().get());

		if (m_AppInfo.IsImGuiEnabled)
		{

			m_RenderContext->ImGuiBeginFrame();

			ImGui::NewFrame();

			OnFrameRenderImGui();

			ImGui::Render();

			m_RenderContext->ImGuiEndFrame();

		}

		On2DRender();

		m_Window->Update();

		Time::EndCPU();

		Time::EndProfile();

		if (LogStutters)
		{
			Z_WARN("Detected stutter! frame got {:.3f}ms over 33ms max delta", FrameDeltaDiff);
			Z_WARN("Logging last frame scope");

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
		LogStutters = false;

	}

	delete scene;
}

void Application::RenderStartupMedia()
{
	EventHandler::InvokeEvent(EventHandler::GetEventID("init_media"), this);

	if (ZVFS::Exists("media/startupvids.txt"))
	{

		std::string path = "media/startupvids.txt";

		RefBinaryStream vids = ZVFS::GetFile(path.c_str());

		std::string line;

		Render::PipelineDescription pipelineDesc;

		pipelineDesc.ShaderPath = "shaders/video_gbar_to_rgba.cso";

		pipelineDesc.VertexLayout = Render::Vertex::GetLayout();

		pipelineDesc.RenderTarget = m_Window->GetFramebuffer().get();

		pipelineDesc.RasterizerState.DepthTest = false;

		pipelineDesc.DepthTargetFormat = Render::ImageFormat::DEPTH16;

		Render::GraphicsPipeline videoPipeline(pipelineDesc);

		auto fullScreenQuad = Render::ShapeProvider::GenerateFullScreenQuad();

		Render::GraphicsCommandBuffer cmdBuffer{};

		while (std::getline(*vids->Stream(), line))
		{
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

				m_Window->SetVSync(false);

				frameIndex = !frameIndex;

				Time::BeginProfile();

				decode.Step();

				JobManager::ExecuteMainJobs();

				accum += Time::DeltaTime;

				cmdBuffer.Begin();

				while (accum >= frameTime && !decode.Finished())
				{
					auto frame = decode.GetFrame();
					if (frame)
					{
						auto cmd = cmdBuffer.GetNativeCommandList();

						cmdBuffer.RequireTextureState(surface.get(), Render::ResourceState::ShaderResource, Render::ResourceState::CopyDest);
						cmdBuffer.CommitBarriers();
						cmd->writeTexture(surface->Handle, 0, 0, frame->native()->data[0], params.width * 4);
						cmdBuffer.RequireTextureState(surface.get(), Render::ResourceState::CopyDest, Render::ResourceState::ShaderResource);
						cmdBuffer.CommitBarriers();

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

				cmdBuffer.RequireFramebufferState(m_Window->GetFramebuffer().get(), Render::ResourceState::Present, Render::ResourceState::RenderTarget);
				cmdBuffer.SetViewport(&vp);
				cmdBuffer.SetPipeline(&videoPipeline);
				cmdBuffer.SetFramebuffer(m_Window->GetFramebuffer().get());
				cmdBuffer.SetTextureResource(surface.get(), 0);
				cmdBuffer.SetTextureSampler(&sampler, 0);

				cmdBuffer.SetVertexBuffer(fullScreenQuad->GetVertexBuffer(), 0);
				cmdBuffer.SetIndexBuffer(fullScreenQuad->GetIndexBuffer());

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
	Z_PROFILE_SCOPE("Application::Update");
	gpGlobals->gametic++;
	OnFrameUpdate();
	Input::Update();
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

	static int frameRate = 0;
	
	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();
}