#include "MainMenuSystem.h"
#include "SparrowReader.h"

#include <Scene/SceneUI.h>
#include <Input/Input.h>
#include <Util/Globals.h>
#include <Event/EventBus.h>
#include <Core/Window.h>

#include "Song/BiteFernanSong.h"
#include "Song/ErectDadBattleSong.h"
#include "LoadingScreenSystem.h"
#include "TimedActionSystem.h"

static Funkin::TimedActionSystem* pTimedActionSystem;
static Stratum::ECS::edict_t camera;
static Stratum::ECS::edict_t bgEntity;

struct MainMenuCharacter
{
	std::string TexturePath;
	std::string SparrowPath;
	std::string AnimName;
	glm::vec2 Position;
	float Scale = 1.0f;
	float FPS = 24.0f;
};

MainMenuCharacter character;
MainMenuCharacter characters[] = {
	{ "ui/menu/char/Nonsense1.png", "ui/menu/char/Nonsense1.xml", "Nonsense", { 900, -250 } },
	{ "ui/menu/char/Nonsense2.png", "ui/menu/char/Nonsense2.xml", "Nonsense", { 950, -300 }, 0.8f },
	{ "ui/menu/char/Nonsense3.png", "ui/menu/char/Nonsense3.xml", "Nonsense", { 900, -430 } },
	{ "ui/menu/char/Nonsense4.png", "ui/menu/char/Nonsense4.xml", "Nonsense", { 700, 100 }, 1.3f },
	{ "ui/menu/char/Nonsense5.png", "ui/menu/char/Nonsense5.xml", "nonsensegod", { 850, 100 } },
	{ "ui/menu/char/Nonsense6.png", "ui/menu/char/Nonsense6.xml", "Nonsense", { 900, 30 }, 0.8f, 26 },
	{ "ui/menu/char/Nonsense14.png", "ui/menu/char/Nonsense14.xml", "Nonsense Bop", { 900, 30 } },
	{ "ui/menu/char/whatIsThis.png", "ui/menu/char/whatIsThis.xml", "that", { 900, 30 }, 2.3f, 27 },
	{ "ui/menu/char/ClassicNonsense.png", "ui/menu/char/ClassicNonsense.xml", "classicNonsense", { 900, 30 }, 1.6f, 24 },
};

enum MenuPanelType
{
	MP_INVALID = -1,
	MP_TITLE,
	MP_MAIN,
	MP_FREEPLAY,
	MP_OPTIONS,
	MP_CREDITS
};

struct ControllerPrompt
{
	Stratum::ECS::edict_t Button;
	Stratum::ECS::edict_t Text;
	int index = 0;
	SDL_GamepadType type = SDL_GAMEPAD_TYPE_MAX;
	bool Active = false;
};

std::vector<ControllerPrompt> gControllerPrompts;

static MenuPanelType gNextPanel = MP_INVALID;
static void SetPanel(MenuPanelType type)
{
	gNextPanel = type;
}

class MenuPanel
{
public:
	virtual void Precache(Stratum::Scene* scene) = 0;
	virtual void Update(Stratum::Scene* scene) = 0;
	virtual void Show(Stratum::Scene* scene) = 0;
	virtual void Hide(Stratum::Scene* scene) = 0;
	virtual void Destroy(Stratum::Scene* scene) = 0;
	virtual bool CanSwap(Stratum::Scene* scene) = 0;
};

class MainMenuPanel : public MenuPanel
{
public:
	Stratum::ECS::edict_t bgChara;
	bool doSwap = false;
	float aliveTime = 0.0f;
	Funkin::MainMenuSystem* pMainSystem;

	MainMenuPanel(Funkin::MainMenuSystem* pSystem)
	{
		pMainSystem = pSystem;
	}

	void Precache(Stratum::Scene* scene) override
	{
		scene->UI->CreateUIPanel("main_menu", "ui/panels/main_menu.json");

		std::vector<std::thread*> threads;

		for (auto chara : characters)
		{
			std::thread* t  = new std::thread([scene, chara]
			{
				scene->Resources.LoadTextureImage(chara.TexturePath);
			});
			threads.push_back(t);
		}

		for (auto t : threads)
		{
			t->join();
			delete t;
		}
	}
	void Update(Stratum::Scene* scene) override
	{
		auto& anchor = scene->GuiAnchors.Get(bgChara);
		anchor.Position.y = character.Position.y + glm::sin(Stratum::gpGlobals->elapsedTime * 1.5f) * 30.0f;
		aliveTime += Stratum::gpGlobals->deltaTime;

		if (Stratum::Input::GetInputDown("menu_back") && aliveTime > 2.5f)
		{
			SetPanel(MP_TITLE);
			pMainSystem->CancelFxSource->Play();
		}
	}
	void Show(Stratum::Scene* scene) override
	{
		pMainSystem->CreateControllerPrompt(GamepadButton::DPAD, L"Navigate");
		pMainSystem->CreateControllerPrompt(GamepadButton::A, L"Select");
		pMainSystem->CreateControllerPrompt(GamepadButton::B, L"Back");
		pMainSystem->CreateControllerPrompt(GamepadButton::BACK, L"Fullscreen");

		doSwap = false;
		aliveTime = 0.0f;
		uint32_t size = sizeof(characters) / sizeof(MainMenuCharacter);

		{
			auto entity = bgChara = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& animator = scene->SpriteAnimators.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			auto chara = characters[(rand() / 10) % size];
			character = chara;

			sprite.RenderLayer = 1;
			sprite.TextureHandle = scene->Resources.LoadTextureImage(chara.TexturePath);
			sprite.Center = { 0.0f, 1.0f };

			anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
			anchor.Position.y = chara.Position.y;
			anchor.Position.x += -3000;

			pTimedActionSystem->PushAction(&anchor.Position.x, chara.Position.x, 1.0f, Funkin::Easing::BackOut);

			auto frames = Funkin::SparrowReader::readXML(chara.SparrowPath, chara.AnimName, false);

			Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(chara.FPS)
				.SetLoop(true)
				.SetTransitionToDefault(true)
				.SetAnimateOnIdle(true)
				.SetFrames(frames);

			animator.AnimationMap["idle"] = animation;
			animator.SetState("idle");
			animator.DefaultAnimation = "idle";

			transform.SetScale(glm::vec3(chara.Scale));
		}

		scene->UI->ShowUIPanel("main_menu");
	}
	void Hide(Stratum::Scene* scene) override
	{
		pMainSystem->ClearControllerPrompts();
		scene->UI->HideUIPanel("main_menu");
		auto& anchor = scene->GuiAnchors.Get(bgChara);
		pTimedActionSystem->PushAction(&anchor.Position.x, -3000.0f, 1.0f, Funkin::Easing::BackIn, [this]
			{
				doSwap = true;
			});
	}
	void Destroy(Stratum::Scene* scene) override
	{
		scene->EntityManager.DestroyEntity(bgChara);
	}
	bool CanSwap(Stratum::Scene* scene) override
	{
		return doSwap;
	}
};

class TitleMenuPanel : public MenuPanel
{
public:
	Stratum::ECS::edict_t whiteFlash;
	Stratum::ECS::edict_t logo;
	Stratum::ECS::edict_t bgChara;
	Stratum::ECS::edict_t bgTrig1;
	Stratum::ECS::edict_t bgTrig2;
	Stratum::ECS::edict_t pressEnter;
	bool doSwap = false;
	bool finishedIntro = false;
	Funkin::MainMenuSystem* pMainSystem;

	float bpm = 102.0f;
	float bps = (bpm / 60.0f);
	float bpmToSeconds = 1.0f / bps;

	TitleMenuPanel(Funkin::MainMenuSystem* pSystem)
	{
		pMainSystem = pSystem;
	}
	void Precache(Stratum::Scene* scene) override
	{
		scene->Resources.LoadTextureImage("ui/triangle.png");
		scene->Resources.LoadTextureImage("ui/title/logoNH.png");
		scene->Resources.LoadTextureImage("ui/title/NonsenseBop.png");
		scene->Resources.LoadTextureImage("ui/title/text.png");
	}
	void Update(Stratum::Scene* scene) override
	{
		
		int32_t beat = glm::floor(bps * pMainSystem->MusicSource->PositionF() + 0.5f);
		static int32_t lastBeat = -1;

		auto& textAnchor = scene->GuiAnchors.Get(pressEnter);
		auto& textTransform = scene->Transforms.Get(pressEnter);
		auto& textSprite = scene->SpriteRenderers.Get(pressEnter);

		auto& logoSprite = scene->SpriteRenderers.Get(logo);
		auto& logoTransform = scene->Transforms.Get(logo);
		logoTransform.IsDirty = true;
		logoSprite.Rotation.x = 8.0f + glm::sin(pMainSystem->MusicSource->PositionF() * bps) * 2.0f;
		auto& cam = scene->Cameras.Get(camera);

		if (lastBeat != beat)
		{
			lastBeat = beat;
			if (beat % 1 == 0)
			{
				pTimedActionSystem->PushAction(&logoTransform.Scale, logoTransform.Scale, bpmToSeconds * 0.9f, Funkin::Easing::ElasticOut);
				logoTransform.Scale += glm::vec3(0.03f);
				cam.OrthographicZoom -= 0.03f;
			}
		}

		auto& animator = scene->SpriteAnimators.Get(bgChara);
		animator.AnimationMap["idle"].FrameIndex = (int)glm::floor(bps * pMainSystem->MusicSource->PositionF() * 14.0f + 7.0f) % 14;

		textTransform.IsDirty = true;

		{
			auto& sprite = scene->SpriteRenderers.Get(pressEnter);
			sprite.SpriteColor.a = glm::sin(Stratum::gpGlobals->elapsedTime * 6.0f) * 0.2f + 0.8f;
		}

		if (Stratum::Input::GetInputDown("menu_accept") && finishedIntro)
		{
			SetPanel(MP_MAIN);
			pMainSystem->ConfirmFxSource->Play();
			Stratum::Input::SetGamepadRumble(0.1f, 0.1f, 1000);

			auto& sprite = scene->SpriteRenderers.Get(whiteFlash);
			sprite.SpriteColor = glm::vec4(1.0f);
			sprite.Enabled = true;
			pTimedActionSystem->PushAction(&sprite.SpriteColor.a, 0.0f, 1.0f, Funkin::Easing::Linear, [this, scene]()
				{
					auto& sprite = scene->SpriteRenderers.Get(whiteFlash);
					sprite.Enabled = false;
					finishedIntro = true;
				});
		}

		if (Stratum::Input::GetInputDown("menu_back"))
		{
			Stratum::EventBus::InvokeEvent(Stratum::ApplicationEvent{ Stratum::ApplicationEvent::APP_EVENT_SHUTDOWN });
		}
	}
	void Show(Stratum::Scene* scene) override
	{
		doSwap = false;
		finishedIntro = false;

		pMainSystem->CreateControllerPrompt(GamepadButton::A, L"Start");
		pMainSystem->CreateControllerPrompt(GamepadButton::B, L"Exit");
		pMainSystem->CreateControllerPrompt(GamepadButton::BACK, L"Fullscreen");

		{
			auto entity = whiteFlash = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);

			sprite.Rect.size = { 10000, 10000 };
			sprite.SpriteColor = glm::vec4(1.0f);
			sprite.RenderLayer = 1000;

			pTimedActionSystem->PushAction(&sprite.SpriteColor.a, 0.0f, 0.5f, Funkin::Easing::Linear, [this, scene]()
				{
					auto& sprite = scene->SpriteRenderers.Get(whiteFlash);
					sprite.Enabled = false;
					finishedIntro = true;
				});
		}

		{
			auto entity = bgChara = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& animator = scene->SpriteAnimators.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			sprite.RenderLayer = 4;
			sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/title/NonsenseBop.png");
			sprite.Center = { 0.0f, 1.0f };

			anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_RIGHT;
			anchor.Position.y = -200.0f;
			anchor.Position.x = 1250.0f;

			auto frames = Funkin::SparrowReader::readXML("ui/title/NonsenseBop.xml", "NonsenseDancemenu", false);

			Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(frames.size() * bps)
				.SetLoop(true)
				.SetTransitionToDefault(true)
				.SetAnimateOnIdle(true)
				.SetFrames(frames);

			animator.AnimationMap["idle"] = animation;
			animator.SetState("idle");
			animator.DefaultAnimation = "idle";
			animator.AnimationMap["idle"].FrameIndex = frames.size() / 2;

			transform.SetScale(glm::vec3(1.0f));
		}

		{
			auto entity = pressEnter = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			sprite.RenderLayer = 10;
			sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/title/text.png");
			sprite.Rect.size = scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();

			anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;
			anchor.Position.y = 200.0f;

			transform.SetScale(glm::vec3(1.0f));
		}

		{
			auto entity = logo = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			transform.SetScale(glm::vec3(0.35f));
			anchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP_LEFT;

			sprite.RenderLayer = 10;
			sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/title/logoNH.png");
			sprite.Rect.size = scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();

			anchor.Position += glm::vec2(sprite.Rect.size) / 2.5f;
		}

		{
			auto entity = bgTrig1 = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			anchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;

			sprite.RenderLayer = 5;
			sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/triangle.png");
			sprite.Rect.size = scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
			sprite.Center = { 0.0f, 1.0f };

			glm::vec3 scale = glm::vec3(1.0f);

			scale.x = scene->VirtualScreenSize.x / sprite.Rect.size.x;

			transform.SetScale(scale);
		}

		{
			auto entity = bgTrig2 = scene->EntityManager.CreateEntity();
			auto& transform = scene->Transforms.Create(entity);
			auto& sprite = scene->SpriteRenderers.Create(entity);
			auto& anchor = scene->GuiAnchors.Create(entity);

			anchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP;

			sprite.RenderLayer = 3;
			sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/triangle.png");
			sprite.Rect.size = scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
			sprite.Center = { 0.0f, 1.0f };
			sprite.FlipY = true;
			sprite.FlipX = true;

			glm::vec3 scale = glm::vec3(1.0f);

			scale.x = scene->VirtualScreenSize.x / sprite.Rect.size.x;

			transform.SetScale(scale);
		}
	}
	void Hide(Stratum::Scene* scene) override
	{
		auto& textAnchor = scene->GuiAnchors.Get(pressEnter);
		auto& logoAnchor = scene->GuiAnchors.Get(logo);
		auto& trig1Anchor = scene->GuiAnchors.Get(bgTrig1);
		auto& trig2Anchor = scene->GuiAnchors.Get(bgTrig2);
		auto& charaAnchor = scene->GuiAnchors.Get(bgChara);

		pTimedActionSystem->PushAction(&textAnchor.Position.y, -500.0f, 1.0f, Funkin::Easing::BackIn);
		pTimedActionSystem->PushAction(&logoAnchor.Position.x, -3000.0f, 1.0f, Funkin::Easing::BackIn);
		pTimedActionSystem->PushAction(&trig1Anchor.Position.x, -3000.0f, 1.0f, Funkin::Easing::SineInOut);
		pTimedActionSystem->PushAction(&trig2Anchor.Position.x, 3000.0f, 1.0f, Funkin::Easing::SineInOut);
		pTimedActionSystem->PushAction(&charaAnchor.Position.x, -3000.0f, 1.0f, Funkin::Easing::BackIn, [this]
			{
				doSwap = true;
			});

		pMainSystem->ClearControllerPrompts();
	}
	void Destroy(Stratum::Scene* scene) override
	{
		scene->EntityManager.DestroyEntity(whiteFlash);
		scene->EntityManager.DestroyEntity(bgChara);
		scene->EntityManager.DestroyEntity(logo);
		scene->EntityManager.DestroyEntity(bgTrig1);
		scene->EntityManager.DestroyEntity(bgTrig2);
		scene->EntityManager.DestroyEntity(pressEnter);
	}
	bool CanSwap(Stratum::Scene* scene) override
	{
		return doSwap;
	}
};

class FreeplayMenuPanel : public MenuPanel
{
public:
	Funkin::MainMenuSystem* pMainSystem;
	FreeplayMenuPanel(Funkin::MainMenuSystem* pSystem)
	{
		pMainSystem = pSystem;
	}
	void Precache(Stratum::Scene* scene)
	{

	}
	void Update(Stratum::Scene* scene)
	{

	}
	void Show(Stratum::Scene* scene)
	{

	}
	void Hide(Stratum::Scene* scene)
	{

	}
	void Destroy(Stratum::Scene* scene)
	{

	}
	bool CanSwap(Stratum::Scene* scene)
	{

	}
};

Stratum::Ref<MenuPanel> gCurrentPanel;
Stratum::Ref<MenuPanel> gPanels[64];

static const std::string ButtonTexturesPS[] =
{
	"ui/prompts/ps/PS3_Cross.png",
	"ui/prompts/ps/PS3_Circle.png",
	"ui/prompts/ps/PS3_Square.png",
	"ui/prompts/ps/PS3_Triangle.png",
	"ui/prompts/ps/PS3_Select.png",
	"ui/prompts/ps/PS3_Start.png",
	"ui/prompts/ps/PS3_Left_Stick_Click.png",
	"ui/prompts/ps/PS3_Right_Stick_Click.png",
	"ui/prompts/ps/PS3_L1.png",
	"ui/prompts/ps/PS3_R1.png",
	"ui/prompts/ps/PS3_Dpad_Up.png",
	"ui/prompts/ps/PS3_Dpad_Down.png",
	"ui/prompts/ps/PS3_Dpad_Left.png",
	"ui/prompts/ps/PS3_Dpad_Right.png",
	"ui/prompts/ps/PS3_Dpad.png",
	"ui/prompts/ps/PS3_Dpad.png"
};

static const std::string ButtonTexturesXbox[] =
{
	"ui/prompts/xbox/360_A.png",
	"ui/prompts/xbox/360_B.png",
	"ui/prompts/xbox/360_X.png",
	"ui/prompts/xbox/360_Y.png",
	"ui/prompts/xbox/360_Back_Alt.png",
	"ui/prompts/xbox/360_Start_Alt.png",
	"ui/prompts/xbox/360_Left_Stick_Click.png",
	"ui/prompts/xbox/360_Right_Stick_Click.png",
	"ui/prompts/xbox/360_LT.png",
	"ui/prompts/xbox/360_RT.png",
	"ui/prompts/xbox/360_Dpad_Up.png",
	"ui/prompts/xbox/360_Dpad_Down.png",
	"ui/prompts/xbox/360_Dpad_Left.png",
	"ui/prompts/xbox/360_Dpad_Right.png",
	"ui/prompts/xbox/360_Dpad.png",
	"ui/prompts/xbox/360_Dpad.png"
};

Funkin::MainMenuSystem::MainMenuSystem()
{
	mScene = NULL;
	gNextPanel = MP_INVALID;
}

Funkin::MainMenuSystem::~MainMenuSystem()
{
	MusicSource->Stop();
	gControllerPrompts.clear();
}

void Funkin::MainMenuSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;

	Stratum::Input::BindAlias("menu_accept", KeyCode::RETURN);
	Stratum::Input::BindAlias("menu_accept", GamepadButton::A);

	Stratum::Input::BindAlias("menu_back", KeyCode::ESCAPE);
	Stratum::Input::BindAlias("menu_back", GamepadButton::B);

	gPanels[MP_TITLE] = Stratum::CreateRef<TitleMenuPanel>(this);
	gPanels[MP_MAIN] = Stratum::CreateRef<MainMenuPanel>(this);

	for (auto& panel : gPanels)
	{
		if (panel)
			panel->Precache(scene);
	}

	for (auto& prompt : ButtonTexturesPS)
	{
		mScene->Resources.LoadTextureImage(prompt);
	}

	for (auto& prompt : ButtonTexturesXbox)
	{
		mScene->Resources.LoadTextureImage(prompt);
	}

	SetPanel(MP_TITLE);

	pTimedActionSystem = new TimedActionSystem();
	scene->RegisterCustomSystem(pTimedActionSystem, true);
	scene->FontRegistry.LoadFont("vcr", "fonts/vcr-org.ttf");

	HoverFxSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/sounds/scrollMenu.mp3", mScene->AudioEngine->GetEngine());
	ConfirmFxSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/sounds/confirmMenu.mp3", mScene->AudioEngine->GetEngine());
	CancelFxSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/sounds/cancelMenu.mp3", mScene->AudioEngine->GetEngine());
	MusicSource = Stratum::CreateRef<Stratum::MP3AudioSource>("fnf/music/freakyMenu.mp3", mScene->AudioEngine->GetEngine());

	mScene->AudioEngine->AddSource(HoverFxSource);
	mScene->AudioEngine->AddSource(ConfirmFxSource);
	mScene->AudioEngine->AddSource(CancelFxSource);
	mScene->AudioEngine->AddSource(MusicSource);

	scene->UI->CreateUIPanel("not_implemented_popup", "ui/panels/not_implemented.json");

	MusicSource->SetLooping(true);

	MusicSource->SetVolume(0.3f);
	HoverFxSource->SetVolume(0.6f);

	{
		auto entity = bgEntity = scene->EntityManager.CreateEntity();
		auto& transform = scene->Transforms.Create(entity);
		auto& sprite = scene->SpriteRenderers.Create(entity);

		sprite.RenderLayer = 0;
		sprite.TextureHandle = scene->Resources.LoadTextureImage("ui/bg_pattern.png");
		sprite.Center = { 0.0f, 0.0f };
		sprite.Rect.position = { 0, 0 };
		sprite.Rect.size = mScene->VirtualScreenSize;

	}

	{
		auto entity = camera = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
	}

	{
		auto entity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
		camera.RenderLayer = 1;
	}

	Stratum::EventBus::RegisterListener<Stratum::AppUIEvent>([this](const Stratum::AppUIEvent& e)
		{
			if (e.EventName == "sfx-ui")
			{
				HoverFxSource->Play();
				Stratum::Input::SetGamepadRumble(0.1f, 0.1f, 50);
				return;
			}
			if (e.EventName == "switch-freeplay-panel")
			{
				using namespace ENGINE_NAMESPACE;

				LoadChartParams params;
				params.ChartPath = "fnf/data/bite/bite-fernan.json";
				//params.ChartPath = "fnf/data/dad-battle/dad-battle-hard.json";
				//params.ChartPath = "fnf/data/erect-dadbattle/erect-dadbattle-erect.json";
				params.SongScript = CreateRef<BiteFernanSong>();
				//params.SongScript = CreateRef<ErectDadBattleSong>();

				auto scene = new Stratum::Scene();
				mScene->SwapScene(scene);

				//scene->RegisterCustomSystem(new BalatroSystem());
				scene->RegisterCustomSystem(new LoadingScreenSystem(params));
				return;
			}
			if (e.EventName == "close-imp")
			{
				mScene->UI->HideUIPanel("not_implemented_popup");
				return;
			}
			mScene->UI->ShowUIPanel("not_implemented_popup");
		}, Stratum::EventFlagBits::EF_REMOVE_ON_SCENE_LOAD);
	
	MusicSource->Play();
}

void Funkin::MainMenuSystem::Update(Stratum::Scene* scene)
{
	if (gNextPanel != MP_INVALID)
	{
		static bool didHide = false;
		if (gCurrentPanel)
		{
			if (!didHide)
			{
				gCurrentPanel->Hide(scene);
				didHide = true;
			}
			if (gCurrentPanel->CanSwap(scene) && gControllerPrompts.empty())
			{
				gCurrentPanel->Destroy(scene);
				gCurrentPanel = gPanels[gNextPanel];
				gCurrentPanel->Show(scene);
				gNextPanel = MP_INVALID;
				didHide = false;
			}
		}
		else
		{
			gCurrentPanel = gPanels[gNextPanel];
			gCurrentPanel->Show(scene);
			gNextPanel = MP_INVALID;
			didHide = false;
		}
	}

	if (gCurrentPanel)
	{
		gCurrentPanel->Update(scene);
	}

	auto& transform = scene->Transforms.Get(camera);
	auto& cam = scene->Cameras.Get(camera);

	cam.OrthographicZoom = glm::mix(cam.OrthographicZoom, 0.95f, Stratum::gpGlobals->deltaTime * 8.0f);

	float hidden = (int)!mScene->UI->IsMouseHidden();

	glm::vec3 pos = transform.Position;
	pos = glm::mix(pos, glm::vec3(mScene->VirtualMousePosition * 0.02f * hidden, 0.0f), Stratum::gpGlobals->deltaTime * 3.0f);

	transform.SetPosition(pos);

	{
		static glm::vec2 pos = {};
		auto& sprite = mScene->SpriteRenderers.Get(bgEntity);
		auto& transform = mScene->Transforms.Get(bgEntity);
		pos += glm::vec2(-20.0f, 20.0f)* Stratum::gpGlobals->deltaTime;
		pos = glm::mod(pos, glm::vec2(150.0f));
		sprite.Rect.position = pos;
		sprite.Rect.size = mScene->VirtualScreenSize * 1.2f;
		transform.SetPosition(glm::vec3(glm::sin(Stratum::gpGlobals->elapsedTime * 1.0f) * 100.0f, 0.0f, 0.0f));
	}

	if (Stratum::Input::GetKeyDown(KeyCode::F11) || Stratum::Input::GetGamepadButtonDown(GamepadButton::BACK))
	{
		static bool fs = false;
		scene->Window->SetFullScreen(fs = !fs);
	}
	
	if (Stratum::Input::HasGamepadConnected())
	{
		uint32_t index = 0;
		for (auto& prompt : gControllerPrompts)
		{
			if (!prompt.Active)
			{
				prompt.Active = true;

				auto& buttonAnchor = mScene->GuiAnchors.Get(prompt.Button);
				auto& textAnchor = mScene->GuiAnchors.Get(prompt.Text);

				pTimedActionSystem->PushAction(&buttonAnchor.Position.x, 70.0f, { 0.5f, 1.0f, index * 0.2f }, Easing::BackOut);
				pTimedActionSystem->PushAction(&textAnchor.Position.x, 120.0f, { 0.5f, 1.0f, index * 0.2f }, Easing::BackOut);

				index++;
			}

			if (prompt.type != Stratum::Input::GetGamepadType())
			{
				prompt.type = (SDL_GamepadType)Stratum::Input::GetGamepadType();

				auto& buttonSprite = mScene->SpriteRenderers.Get(prompt.Button);

				if (prompt.type == SDL_GAMEPAD_TYPE_PS3 || prompt.type == SDL_GAMEPAD_TYPE_PS4 || prompt.type == SDL_GAMEPAD_TYPE_PS5)
				{
					buttonSprite.TextureHandle = mScene->Resources.LoadTextureImage(ButtonTexturesPS[prompt.index]);
				}

				if (prompt.type == SDL_GAMEPAD_TYPE_XBOX360 || prompt.type == SDL_GAMEPAD_TYPE_XBOXONE || prompt.type == SDL_GAMEPAD_TYPE_UNKNOWN || prompt.type == SDL_GAMEPAD_TYPE_STANDARD)
				{
					buttonSprite.TextureHandle = mScene->Resources.LoadTextureImage(ButtonTexturesXbox[prompt.index]);
				}

				buttonSprite.Rect.size = mScene->Resources.GetImageHandle(buttonSprite.TextureHandle)->GetSize();
			}
		}
	}
	else
	{
		uint32_t index = 0;
		for (auto& prompt : gControllerPrompts)
		{
			if (prompt.Active)
			{
				prompt.Active = false;

				auto& buttonAnchor = mScene->GuiAnchors.Get(prompt.Button);
				auto& textAnchor = mScene->GuiAnchors.Get(prompt.Text);

				pTimedActionSystem->PushAction(&buttonAnchor.Position.x, -700, { 0.5f, 1.0f, index * 0.2f }, Easing::BackIn);
				pTimedActionSystem->PushAction(&textAnchor.Position.x, -700 + 70, { 0.5f, 1.0f, index * 0.2f }, Easing::BackIn);

				index++;
			}
		}
	}
}

void Funkin::MainMenuSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Funkin::MainMenuSystem::RenderImGui(Stratum::Scene* scene)
{

}

void Funkin::MainMenuSystem::CreateControllerPrompt(GamepadButton button, const std::wstring& text)
{
	auto buttonPrompt = mScene->EntityManager.CreateEntity();
	auto& buttonSprite = mScene->SpriteRenderers.Create(buttonPrompt);
	auto& buttonTransform = mScene->Transforms.Create(buttonPrompt);
	auto& buttonAnchor = mScene->GuiAnchors.Create(buttonPrompt);

	auto textEntity = mScene->EntityManager.CreateEntity();
	auto& textComp = mScene->TextComponents.Create(textEntity);
	auto& textComponent = mScene->TextComponents.Create(textEntity);
	auto& textRenderer = mScene->TextRenderers.Create(textEntity);
	auto& textTransform = mScene->Transforms.Create(textEntity);
	auto& textAnchor = mScene->GuiAnchors.Create(textEntity);

	textComponent.Font = "vcr";
	textComponent.FontSize = 64.0f;
	textComponent.Text = text;

	buttonTransform.SetScale(glm::vec3(0.5f));
	textTransform.SetPosition({ 0.0f, -24.0f, 0.0f });

	buttonSprite.RenderLayer = 90000000; // Over 9000!
	buttonSprite.CameraLayer = 1;
	textRenderer.RenderLayer = buttonSprite.RenderLayer;
	textRenderer.CameraLayer = 1;

	textAnchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP_LEFT;
	buttonAnchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP_LEFT;

	int index = gControllerPrompts.size();

	buttonAnchor.Position.y = 70.0f + index * 90.0f;
	buttonAnchor.Position.x = -700.0f;
	textAnchor.Position.x = buttonAnchor.Position.x + 70.0f;
	textAnchor.Position.y = buttonAnchor.Position.y + 24.0f;

	gControllerPrompts.push_back({ buttonPrompt, textEntity, static_cast<int>(button) });
}

void Funkin::MainMenuSystem::ClearControllerPrompts()
{
	uint32_t index = 0;
	for (auto& prompt : gControllerPrompts)
	{
		auto& buttonAnchor = mScene->GuiAnchors.Get(prompt.Button);
		auto& textAnchor = mScene->GuiAnchors.Get(prompt.Text);

		pTimedActionSystem->PushAction(&buttonAnchor.Position.x, -700, { 0.5f, 1.0f, index * 0.2f }, Easing::BackIn);
		pTimedActionSystem->PushAction(&textAnchor.Position.x, -700 + 70, { 0.5f, 1.0f, index * 0.2f }, Easing::BackIn, [this, prompt, index]
			{
				mScene->EntityManager.DestroyEntity(prompt.Button);
				mScene->EntityManager.DestroyEntity(prompt.Text);

				if (index == gControllerPrompts.size() - 1)
					gControllerPrompts.clear();
			});

		index++;
	}
}
