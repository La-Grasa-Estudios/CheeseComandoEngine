#include "EntryPoint.h"
#include "JavosApp.h"

using namespace ENGINE_NAMESPACE;

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = false;
	info.VSyncEnabled = false;
	info.ShouldWindowStartMaximized = true;
	info.ShouldWindowNotResize = true;
	info.WindowName = "Javos Mod";
	info.WindowedResolutionX = 1600;
	info.WindowedResolutionY = 900;
	info.graphicsAPI = Render::RendererAPI::DX12;

	for (auto arg : args)
	{
		if (arg.compare("-vulkan") == 0)
		{
			info.graphicsAPI = Render::RendererAPI::VULKAN;
		}
	}

	Application* app = new Funkin::JavosApp(info);
	return app;
}