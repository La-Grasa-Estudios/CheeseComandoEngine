#include "Engine/EntryPoint.h"

using namespace ENGINE_NAMESPACE;

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = true;
	info.VSyncEnabled = true;
	info.WindowName = "Stratum";
	info.WindowedResolutionX = 1600;
	info.WindowedResolutionY = 900;

	Application* app = new Application(info);
	return app;
}