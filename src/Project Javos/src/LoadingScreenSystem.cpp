#include "LoadingScreenSystem.h"

#include "InGameSystem.h"
#include "SparrowReader.h"

#include <Core/Logger.h>
#include <Core/Application.h>
#include <Input/Input.h>
#include <Util/Globals.h>
#include <Thirdparty/imgui/imgui.h>

// Little engine hack to access current app
extern Stratum::Application* g_CurrentApp;

// Ideally these should be part of the class instance
// Not actually needed since its guaranteed that there is only one instance of the class
Funkin::InGameSystem* pIngameSystem;
Stratum::Scene* pIngameScene;
Stratum::ECS::edict_t loadingProgressEntity;
Stratum::ECS::edict_t bgEntity;
Stratum::ECS::edict_t pressEnterEntity;
float gColor = 1.0f;
bool doTransition = false;

Funkin::LoadingScreenSystem::LoadingScreenSystem(const LoadChartParams& params)
{
	mLoadParams = params;
	mLoadingScene = NULL;
	mScene = NULL;
}

Funkin::LoadingScreenSystem::~LoadingScreenSystem()
{
	mMusicSource->Stop();
}

void Funkin::LoadingScreenSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;
	gColor = 1.0f;
	doTransition = false;

	loadingProgressEntity = scene->EntityManager.CreateEntity();

	{
		bgEntity = scene->EntityManager.CreateEntity();

		auto& transform = scene->Transforms.Create(bgEntity);
		auto& sprite = scene->SpriteRenderers.Create(bgEntity);

		sprite.TextureHandle = scene->Resources.LoadTextureImage("fnf/images/loadingScreen/funkin.png");
		sprite.Rect.size = scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
	}

	{
		pressEnterEntity = scene->EntityManager.CreateEntity();
		auto& transform = scene->Transforms.Create(pressEnterEntity);
		auto& sprite = scene->SpriteRenderers.Create(pressEnterEntity);
		auto& animator = scene->SpriteAnimators.Create(pressEnterEntity);
		auto& anchor = scene->GuiAnchors.Create(pressEnterEntity);

		sprite.RenderLayer = 1;
		sprite.TextureHandle = scene->Resources.LoadTextureImage("fnf/images/loadingScreen/pressEnter.png");
		sprite.Center = { 0.0f, 1.0f };

		anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;
		anchor.Position.y += 100;

		auto frames = SparrowReader::readXML("fnf/images/loadingScreen/pressEnter.xml", "press to start", false);

		Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetLoop(false)
			.SetTransitionToDefault(false)
			.SetAnimateOnIdle(false)
			.SetFrames(frames);

		animator.AnimationMap["press"] = animation;
		animator.SetState("press");
	}

	auto& transform = scene->Transforms.Create(loadingProgressEntity);
	auto& sprite = scene->SpriteRenderers.Create(loadingProgressEntity);
	auto& anchor = scene->GuiAnchors.Create(loadingProgressEntity);

	anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_RIGHT;
	anchor.Position = { 100.0f, 100.0f };

	sprite.Rect.size = { -1, 20 };
	sprite.Center = { 1.0f, 0.0f };
	sprite.RenderLayer = 10;

	pIngameSystem = new InGameSystem(mLoadParams);

	// TO DO: Allow replacing music per song
	mMusicSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/music/loadingThemeLol.mp3", scene->AudioEngine->GetEngine());
	scene->AudioEngine->AddSource(mMusicSource);
	mMusicSource->SetLooping(true);
	mMusicSource->Play();

	std::thread* loadingThread = new std::thread([this]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Sleep, maybe fixes race conditions on other pcs?

			auto scene = new Stratum::Scene();
			pIngameScene = scene;

			g_CurrentApp->InitSceneResources(scene);

			scene->RegisterCustomSystem(pIngameSystem, true);
		});

#ifdef _DEBUG
	if (Stratum::Render::RendererContext::get_api() == Stratum::Render::RendererAPI::VULKAN)
	{
		loadingThread->join();
	}
#endif
}

void Funkin::LoadingScreenSystem::Update(Stratum::Scene* scene)
{

	float targetScale = (float)scene->VirtualScreenSize.x - 100.0f;

	auto& transform = scene->Transforms.Get(loadingProgressEntity);
	transform.Scale.x = glm::mix(transform.Scale.x, targetScale * pIngameSystem->GetLoadingProgress(), 5.0f * Stratum::gpGlobals->deltaTime);
	transform.IsDirty = true;

	auto& sprite1 = scene->SpriteRenderers.Get(pressEnterEntity);
	auto& sprite2 = scene->SpriteRenderers.Get(bgEntity);

	if (pIngameSystem->IsLoadingDone() && glm::abs(transform.Scale.x - targetScale) <= 2.0f)
	{
		if (!sprite1.Enabled)
		{
			auto& animator = scene->SpriteAnimators.Get(pressEnterEntity);
			animator.SetState("press");
		}

		sprite1.Enabled = true;

		auto& sprite = scene->SpriteRenderers.Get(loadingProgressEntity);
		sprite.Enabled = false;

		if (Stratum::Input::GetKeyDown(KeyCode::RETURN) || Stratum::Input::AnyGamepadDown())
		{
			doTransition = true;
		}
	}
	else
	{
		sprite1.Enabled = false;
	}

	// Sweet fade to black transition
	if (doTransition)
	{
		gColor -= Stratum::gpGlobals->deltaTime;
		if (gColor < -1.0f)
		{
			scene->SwapScene(pIngameScene);
		}

		float c = gColor < 0.0f ? 0.0f : gColor;

		auto& sprite = scene->SpriteRenderers.Get(loadingProgressEntity);

		sprite.SpriteColor = glm::vec4(c, c, c, 1.0f);
		sprite1.SpriteColor = glm::vec4(c, c, c, 1.0f);
		sprite2.SpriteColor = glm::vec4(c, c, c, 1.0f);
		mMusicSource->SetVolume(c);
	}

	{
		auto& transform = scene->Transforms.Get(bgEntity);
		auto& sprite = scene->SpriteRenderers.Get(bgEntity);

		glm::vec2 v = sprite.Rect.size;
		v = scene->VirtualScreenSize / v;

		transform.Scale = { v, 0.0f };
		transform.IsDirty = true;
	}
}

void Funkin::LoadingScreenSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Funkin::LoadingScreenSystem::RenderImGui(Stratum::Scene* scene)
{

}