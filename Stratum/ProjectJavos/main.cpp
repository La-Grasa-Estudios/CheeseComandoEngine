#include "EntryPoint.h"
#include "JavosApp.h"

using namespace ENGINE_NAMESPACE;

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = true;
	info.VSyncEnabled = true;
	info.ShouldWindowStartMaximized = true;
	info.ShouldWindowNotResize = true;
	info.WindowName = "Javos Mod";
	info.WindowedResolutionX = 1600;
	info.WindowedResolutionY = 900;

	Application* app = new Javos::JavosApp(info);
	return app;
}