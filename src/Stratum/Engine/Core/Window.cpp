#include "Window.h"

#include "Renderer/RendererContext.h"
#include "Input/Input.h"
#include "VarRegistry.h"
#include "Logger.h"
#include "Event/EventBus.h"

#include <vector>
#include "Thirdparty/imgui/imgui_impl_sdl3.h"

#include <iostream>

using namespace ENGINE_NAMESPACE;

using namespace Internal;

Window::Window(Render::RendererContext* pContext, const char* name)
{
	m_Window = NULL;
	this->m_Name = name;
	this->m_Context = pContext;
	this->m_Vsync = false;
}

Internal::Window::~Window()
{
	if (!m_Window) return;
}

void Window::Create(int width, int height)
{
	VarRegistry::RegisterConsoleVar("cl", "width", VarType::Int)->set(width);
	VarRegistry::RegisterConsoleVar("cl", "height", VarType::Int)->set(height);

	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		// TO DO: Set up a proper warning
		Z_ERROR("Could not initialize SDL! SDL_Error: {}", SDL_GetError());
		return;
	}

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD))
	{
		Z_ERROR("Could not initialize SDL Gamepad! SDL_Error: {}", SDL_GetError());
		return;
	}

	s_instance = this;

	uint32_t flags = 0;
	if (this->StartMaximized) {
		flags |= SDL_WINDOW_MAXIMIZED;
	}
	if (this->FullScreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
		m_IsWindowFullScreen = true;
	}

	m_Window = SDL_CreateWindow(this->m_Name, width, height, flags);
	if (!m_Window)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return;
	}

	m_Context->initialize(this);

	Render::FramebufferDesc framebufferDesc;
	framebufferDesc.IsWindowSurfaceFb = true;

	m_Framebuffer = CreateRef<Render::Framebuffer>(framebufferDesc);

	m_Framebuffer->m_Width = GetWidth();
	m_Framebuffer->m_Height = GetHeight();

}

void Internal::Window::Destroy()
{
	m_Context->Terminate();
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
	m_Window = NULL;
}

int Window::GetWidth()
{
	int width, h;
	SDL_GetWindowSize(m_Window, &width, &h);
	return width;
}

int Window::GetHeight()
{
	int height, w;
	SDL_GetWindowSize(m_Window, &w, &height);
	return height;
}

SDL_Window* Window::GetHandle()
{
	return m_Window;
}

Ref<Render::Framebuffer> Internal::Window::GetFramebuffer()
{
	return m_Framebuffer;
}

bool Window::CloseRequested()
{
	return m_ShouldClose;
}

bool Internal::Window::IsWindowActive()
{
	return m_IsWindowFocused;
}

void ENGINE_NAMESPACE::Internal::Window::Clear()
{
	m_Context->clear_front_buffer((int)Render::ClearBits::COLOR_BIT | (int)Render::ClearBits::DEPTH_BIT);
}

void Window::Update()
{
	m_Framebuffer->m_Width = GetWidth();
	m_Framebuffer->m_Height = GetHeight();

	m_Context->present(this);
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (m_IsImGuiEnabled) {
			ImGui_ImplSDL3_ProcessEvent(&e);
		}
		switch (e.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		case SDL_EVENT_QUIT:
			EventBus::InvokeEvent(ApplicationEvent{ ApplicationEvent ::APP_EVENT_SHUTDOWN});
			break;
		case SDL_EVENT_WINDOW_RESIZED:
		{
			int width = e.window.data1;
			int height = e.window.data2;
			ConsoleVar* pWidthVar = VarRegistry::GetConsoleVar("cl", "width");
			ConsoleVar* pHeightVar = VarRegistry::GetConsoleVar("cl", "height");
			pWidthVar->set(width);
			pHeightVar->set(height);
			std::string log;
			VarRegistry::ParseConsoleVar("r_video_reset", log);
		}
			break;
		default:
			EventBus::InvokeEvent(ApplicationSDLEvent{ this, &e });
			break;
		}
	}
	
	m_IsWindowFocused = (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_INPUT_FOCUS);
}

void Internal::Window::ResetViewport()
{
	m_Context->set_viewport(this->GetWidth(), this->GetHeight());
}

void ENGINE_NAMESPACE::Internal::Window::SetName(const char* name)
{
	SDL_SetWindowTitle(m_Window, name);
}

void ENGINE_NAMESPACE::Internal::Window::SetIcon(int count, WindowIcon* icons)
{
	
}

void Internal::Window::SetVSync(bool vsync)
{
	m_Vsync = vsync;
	m_Context->set_vsync(vsync);
}

void Internal::Window::SetFullScreen(bool fs, VideoDisplayMode* dp)
{
	if (m_IsWindowFullScreen != fs)
	{
		m_IsWindowFullScreen = fs;

		if (dp)
		{
			auto displayId = SDL_GetDisplayForWindow(m_Window);

			auto mode = SDL_GetClosestFullscreenDisplayMode(displayId, dp->Width, dp->Height, static_cast<float>(dp->RefreshRate), SDL_FALSE);;
						
			SDL_SetWindowFullscreenMode(m_Window, mode);
		}
		else
		{
			SDL_SetWindowFullscreenMode(m_Window, NULL);
		}

		SDL_SetWindowFullscreen(m_Window, (SDL_bool)m_IsWindowFullScreen);
	}
}

void Internal::Window::SetInfo(WindowEnum param, bool val)
{
	switch (param)
	{
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_NULL:
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_START_MAXIMIZED:
		StartMaximized = val;
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_FULLSCREEN:
		FullScreen = val;
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_IMGUI:
		m_IsImGuiEnabled = val;
		break;
	case ENGINE_NAMESPACE::Internal::WindowEnum::WINDOW_VULKAN:
		m_VulkanCapable = true;
		return;
	default:
		break;
	}
}

std::vector<VideoDisplayMode> Internal::Window::GetDisplayModes()
{
	auto displayId = SDL_GetDisplayForWindow(m_Window);

	int count;
	auto modes = SDL_GetFullscreenDisplayModes(displayId, &count);

	std::vector<VideoDisplayMode> modesArray;

	for (int i = 0; i < count; i++)
	{
		VideoDisplayMode mode{ modes[i]->format, static_cast<uint32_t>(modes[i]->w), static_cast<uint32_t>(modes[i]->h), static_cast<uint32_t>(modes[i]->refresh_rate) };
		modesArray.push_back(mode);
	}

	return modesArray;
}
