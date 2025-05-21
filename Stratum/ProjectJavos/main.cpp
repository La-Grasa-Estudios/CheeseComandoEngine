#include "EntryPoint.h"
#include "JavosApp.h"

using namespace ENGINE_NAMESPACE;

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = true;
	info.VSyncEnabled = true;
	info.ShouldWindowStartMaximized = false;
	info.WindowName = "Javos Mod";
	info.WindowedResolutionX = 1280;
	info.WindowedResolutionY = 720;

	Application* app = new Javos::JavosApp(info);
	return app;
}