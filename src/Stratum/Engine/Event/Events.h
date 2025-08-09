#pragma once

#include <znmsp.h>

BEGIN_ENGINE

struct EngineModuleInitEvent
{
	enum EngineModule
	{
		ENGINE_MODULE_WINDOW,
		ENGINE_MODULE_INPUT,
		ENGINE_MODULE_AUDIO,
		ENGINE_MODULE_MEDIA,
		ENGINE_MODULE_LATE_INIT,
		ENGINE_MODULE_POST_INIT,
	};
	EngineModule Module;
	EngineModuleInitEvent(EngineModule module)
		: Module(module) {
	}
};

struct ApplicationEvent
{
	enum 
	{
		APP_EVENT_INIT,
		APP_EVENT_SHUTDOWN,
	} Type;
};

struct ApplicationSDLEvent
{
	void* pWindow; // The Engine Window handle, cast this to Stratum::Window*
	void* pEventData; // Cast this to SDL_Event*
};

END_ENGINE