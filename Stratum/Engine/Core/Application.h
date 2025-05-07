#pragma once

#include "znmsp.h"

#include "Ref.h"
#include "Window.h"
#include "Console.h"

#include "Sound/AudioEngine.h"

#include "Renderer/RendererContext.h"

BEGIN_ENGINE

/// <summary>
/// App Info
/// Recommended to memset this with 0 bytes
/// </summary>


struct ApplicationInfo {

	bool VSyncEnabled = false; /// Is Vsync Enabled?
	bool Fullscreen = false; /// Start in fullscreen mode

	bool ShouldWindowStartMaximized = false; // Maximize window on start
	bool ShouldWindowNotResize = false; /// Is window allowed to be resized?

	bool IsImGuiEnabled = false; /// Enable ImGUI

	int WindowedResolutionX = 0; /// Window Res X (Required)
	int WindowedResolutionY = 0; /// Window Res Y (Required)

	const char* WindowName = ""; /// Window Name (Required)

	Render::RendererAPI graphicsAPI; // Graphics API (Default = D3D12)

};

/// <summary>
/// Base App Class
/// Override the
/// OnInit(), OnFrameUpdate() and OnFrameRender() methods
/// </summary> 

class Application {

public:

	inline static std::atomic_uint64_t s_RenderThreadStatus;
	inline static Application* s_Instance = NULL;

	Application(ApplicationInfo& appInfo);

	void Run(std::vector<std::string> args);

	void MainLoop();
	void RenderStartupMedia();

	void InternalUpdate();

	virtual void OnEarlyInit();
	virtual void OnInit();
	virtual void OnFrameUpdate();

	virtual void OnFrameSync();
	virtual void OnFramePrepare();
	virtual void OnFrameRender();

	virtual void On2DRender();
	virtual void OnFrameRenderImGui();

protected:

	ApplicationInfo m_AppInfo;

	Scope<Internal::Window> m_Window;
	Ref<Render::RendererContext> m_RenderContext;

	Ref<AudioEngine> m_AudioEngine;

	Console m_Console{};

private:

};

END_ENGINE