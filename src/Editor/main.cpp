#include "EntryPoint.h"

#include <Scene/Scene.h>
#include "Systems/CardSystem.h"

using namespace ENGINE_NAMESPACE;

static CardSystem* gCardSystem = nullptr;

class App : public Application
{
public:
	App(ApplicationInfo& info) : Application(info)
	{
		
	}

	void OnInit() override
	{
		gCardSystem = new CardSystem();

		auto scene = new Scene();
		SetScene(scene);

		scene->RegisterCustomSystem(gCardSystem, true);

		auto entity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.Orthographic = true;
		camera.RendersToGui = true;

		for (int i = 0; i < 5; i++)
		{
			auto card = gCardSystem->CreateCard((CardType)(i % 4));

			gCardSystem->SetCardInHand(card, true, i);
		}

		auto outputEntity = scene->EntityManager.CreateEntity();
		scene->Transforms.Create(outputEntity);
		auto& anchor = scene->GuiAnchors.Create(outputEntity);
		anchor.AnchorPoint = GuiAnchorPoint::TOP_LEFT;
		anchor.Position = glm::vec2(100.0f, 100.0f);
		auto& text = scene->TextComponents.Create(outputEntity);
		auto& textRenderer = scene->TextRenderers.Create(outputEntity);
		text.Text = L"Output: 0/0";
		text.FontSize = 64.0f;
	}
};

Application* AppMain(std::vector<std::string> args)
{
	ApplicationInfo info;

	info.IsImGuiEnabled = true;
	info.VSyncEnabled = true;
	info.ShouldWindowStartMaximized = false;
	info.ShouldWindowNotResize = false;
	info.WindowName = "Into The Kernel";
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