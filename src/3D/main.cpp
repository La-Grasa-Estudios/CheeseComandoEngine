#include "EntryPoint.h"

#include <Scene/Scene.h>

using namespace ENGINE_NAMESPACE;

class App : public Application
{
public:
	App(ApplicationInfo& info) : Application(info)
	{
		
	}

	void OnInit() override
	{
		auto scene = new Scene();
		SetScene(scene);

		auto entity = scene->EntityManager.CreateEntity();
		auto& transform = scene->Transforms.Create(entity);
	}
};

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = false;
	info.VSyncEnabled = true;
	info.ShouldWindowStartMaximized = true;
	info.ShouldWindowNotResize = false;
	info.WindowName = "Deferred Test";
	info.WindowedResolutionX = 1280;
	info.WindowedResolutionY = 720;
	info.graphicsAPI = Render::RendererAPI::DX12;

	for (auto arg : args)
	{
		if (arg.compare("-vulkan") == 0)
		{
			info.graphicsAPI = Render::RendererAPI::VULKAN;
		}
	}

	Application* app = new App(info);
	return app;
}