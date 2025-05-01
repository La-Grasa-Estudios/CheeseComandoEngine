#pragma once

#include "znmsp.h"

#include "Renderer/Framebuffer.h"
#include <SDL3/SDL.h>

BEGIN_ENGINE

namespace Render {
	class RendererContext;
}

struct WindowIcon {
	int width;
	int height;
	unsigned char* pixels;
};

namespace Internal {

	enum class WindowEnum {
		WINDOW_NULL,
		WINDOW_START_MAXIMIZED,
		WINDOW_FULLSCREEN,
		WINDOW_IMGUI,
	};

	class Window
	{

	public:

		inline static Window* s_instance = 0;

		Window(Render::RendererContext* pContext, const char* name);
		~Window();

		void Create(int width, int height);
		void Destroy();

		int GetWidth();
		int GetHeight();

		SDL_Window* GetHandle();
		Render::RendererContext* GetContext() { return m_Context; }
		std::string GetName() { return m_Name; }
		Ref<Render::Framebuffer> GetFramebuffer();

		bool CloseRequested();
		bool IsWindowActive();

		void Clear();
		void Update();
		void ResetViewport();

		void SetName(const char* name);
		void SetIcon(int count, WindowIcon* icons);
		void SetVSync(bool vsync);

		void SetInfo(WindowEnum param, bool val);

	private:

		const char* m_Name;

		bool StartMaximized = false;
		bool FullScreen = false;
		bool m_Vsync;
		bool m_IsWindowFocused = false;
		bool m_IsImGuiEnabled = false;
		bool m_ShouldClose = false;
		SDL_Window* m_Window;
		SDL_Surface* m_Surface;
		Render::RendererContext* m_Context;
		Ref<Render::Framebuffer> m_Framebuffer;

	};
}

END_ENGINE

