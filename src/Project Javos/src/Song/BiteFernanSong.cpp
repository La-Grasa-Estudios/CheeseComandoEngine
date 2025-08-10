#include "SongCommon.h"
#include "BiteFernanSong.h"

#include <Core/Time.h>
#include <Core/VarRegistry.h>
#include <Util/Globals.h>
#include <Util/StrUtil.h>

#undef min
#undef max

static Stratum::ECS::edict_t startupVideo = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBar = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBarBg = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeBarLoadedBg = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeDuration = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeHud = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t youtubeFade = Stratum::ECS::C_INVALID_ENTITY;
static Stratum::ECS::edict_t loadingYoutubeEntity = Stratum::ECS::C_INVALID_ENTITY;

extern Funkin::GameState gGameState;

void Funkin::BiteFernanSong::Init(Conductor* pConductor, InGameSystem* pIngameSystem, Stratum::Scene* pScene)
{
	// pIngameSystem->CameraZoomModifier = 0.4f;

	startupVideo = 0;
	mConductor = pConductor;
	mIngameSystem = pIngameSystem;
	mScene = pScene;

	mIngameSystem->TrackPlayersEnabled = false;

	StageRegistry::AddStage("syobon1-4");
	StageRegistry::AddStage("syobon1-1");
	StageRegistry::AddStage("syobon-end");

	CharaRegistry::AddCharacter("Syobon");
	CharaRegistry::AddCharacter("syobon");
	CharaRegistry::AddCharacter("cebollaconpelo");
	CharaRegistry::AddCharacter("Fernan");
	CharaRegistry::AddCharacter("FernanRene");
	CharaRegistry::AddCharacter("FernanBebe");
	CharaRegistry::AddCharacter("FernanJumpeado");
	CharaRegistry::AddCharacter("rene");

	CharaRegistry::GetCharacter("Syobon")->AddAnimation("die", "Syobon Die", "", 30, true);

	CharaRegistry::GetCharacter("Fernan")->AddAnimation("leftAlt", "Fernan Left Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("rightAlt", "Fernan Right Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("upAlt", "Fernan Up Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("downAlt", "Fernan Down Alt", "idle", 30, false);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("ahh", "Fernan Goku2", "", 30, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues", "Fernan Cry3", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues1", "Fernan Cry1", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("llorapues2", "Fernan Cry2", "", 8, false, true);
	CharaRegistry::GetCharacter("Fernan")->AddAnimation("WTF", "Fernan Left Alt", "", 8, false, true);

	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("no", "Fernan Empieza", "", 8, false, true);
	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("nO", "Fernan No0", "", 8, false, true);
	CharaRegistry::GetCharacter("FernanJumpeado")->AddAnimation("NO", "Fernan NOO", "", 8, false, true);

	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("eh", "Fernan Huh", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("nosabe", "Fernan Jejnosab", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->AddAnimation("no", "Fernan Nono", "", 30, true);
	CharaRegistry::GetCharacter("FernanRene")->SetCenter({});

	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("repeat", "Fernan Beberepeat", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("nopodemos", "Fernan Nopodemos", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("sipodemos", "Fernan Sipodemos", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("queno", "Fernan Queno", "", 60, true);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("waa", "Fernan Waaa", "", 60, false);
	CharaRegistry::GetCharacter("FernanBebe")->AddAnimation("uahh", "Fernan UAHH", "", 60, true);

	mScene->FontRegistry.LoadFont("Youtube", "fonts/YoutubeSans-Titles.ttf");

	ChartEvent EventSyobon{};
	ChartEvent EventCebolla{};
	ChartEvent Event1dash4{};
	ChartEvent Event1dash2{};
	ChartEvent Event1dash1{};
	ChartEvent EventWhite{};
	ChartEvent EventStopWhite{};
	ChartEvent EventStartBlack{};
	ChartEvent EventStartBlack1{};
	ChartEvent EventStopBlack{};

	Event1dash4.EventName = "StSetStage";
	Event1dash2.EventName = "StSetStage";
	Event1dash1.EventName = "StSetStage";

	EventWhite.EventName = "StFadeToWhite";
	EventStopWhite.EventName = "StFadeToWhite";

	EventStartBlack.EventName = "StFadeToBlack";
	EventStartBlack1.EventName = "StFadeToBlack";
	EventStopBlack.EventName = "StFadeToBlack";

	EventSyobon.EventName = "StSetPlayerCharacter";
	EventCebolla.EventName = "StSetPlayerCharacter";

	EventSyobon.Arg1 = "syobon";
	EventCebolla.Arg1 = "cebollaconpelo";

	EventStartBlack1.EventTime = 109.75f;
	EventStartBlack.EventTime = 147.45f;
	EventStopBlack.EventTime = 149.65f;

	EventStartBlack.Arg1 = "1.0";
	EventStartBlack.Arg2 = "1";

	EventStartBlack1.Arg1 = "1.0";
	EventStartBlack1.Arg2 = "1";

	EventStopBlack.Arg1 = "0.0";
	EventStopBlack.Arg2 = "1";

	Event1dash4.EventTime = 110.82f;
	EventSyobon.EventTime = 110.82f;
	Event1dash2.EventTime = 149.65f;
	EventCebolla.EventTime = 149.65f;

	Event1dash1.EventTime = 232.49f;

	EventWhite.EventTime = 110.82f;
	EventStopWhite.EventTime = 110.9f;

	EventWhite.Arg1 = "1.0";
	EventWhite.Arg2 = "1";

	EventStopWhite.Arg1 = "0.0";
	EventStopWhite.Arg2 = "1000";

	Event1dash2.Arg1 = "syobon";
	Event1dash4.Arg1 = "syobon1-4";
	Event1dash1.Arg1 = "syobon1-1";

	mConductor->chart.events.push_back(Event1dash4);
	mConductor->chart.events.push_back(Event1dash2);
	mConductor->chart.events.push_back(EventWhite);
	mConductor->chart.events.push_back(EventStopWhite);
	mConductor->chart.events.push_back(EventStartBlack1);
	mConductor->chart.events.push_back(EventStartBlack);
	mConductor->chart.events.push_back(EventStopBlack);
	mConductor->chart.events.push_back(EventSyobon);
	mConductor->chart.events.push_back(EventCebolla);
	mConductor->chart.events.push_back(Event1dash1);

	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen2.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/holasaul.png");
	mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/loadingYoutube.png");

	pTimedActionSystem = pIngameSystem->pTimedActionSystem;

	static Stratum::ECS::edict_t x64entity;
	static Stratum::ECS::edict_t x64blackentity;

	youtubeDuration = pIngameSystem->CreateTextEntity(L"0:00", { 0.0f, 0.0f }, 50.0f, true, 901, 0.0f);
	auto& youtubeAnchor = mScene->GuiAnchors.Create(youtubeDuration);
	youtubeAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeAnchor.Position = { 500, 67.0f };

	youtubeHud = pIngameSystem->CreateSpriteEntity("fnf/SyobonNoAction/videohud.png", { 0.0f, 0.0f }, { 1.0f, 1.0f }, true, 900);
	youtubeFade = pIngameSystem->CreateSpriteEntity("fnf/SyobonNoAction/black.png", { 0.0f, 0.0f }, { 1.0f, 1.0f }, true, 899);
	youtubeBar = pIngameSystem->CreateRectEntity({ 0.0f, 0.0f }, { 100.0f, 10.0f }, { -1.0f, 1.0f }, true, 902);
	youtubeBarBg = pIngameSystem->CreateRectEntity({ 0.0f, 0.0f }, { 100.0f, 10.0f }, { -1.0f, 1.0f }, true, 901);

	auto youtubeTitle = pIngameSystem->CreateTextEntity(L"Gato Bros (Syobon Action) en español por fernanfloo", { 0.0f, 0.0f }, 70.0f, true, 900, 0.0f);
	auto& youtubeTitleAnchor = mScene->GuiAnchors.Create(youtubeTitle);
	youtubeTitleAnchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP_LEFT;
	youtubeTitleAnchor.Position = { 35.0f, 100.0f };
	mScene->TextComponents.Get(youtubeTitle).Font = "Youtube";

	auto& transform = mScene->Transforms.Get(youtubeBar);
	transform.Scale.x = 0.0f;

	auto& youtubeBarAnchor = mScene->GuiAnchors.Create(youtubeBar);
	youtubeBarAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeBarAnchor.Position = { 25.0f, 165.0f };

	auto& youtubeBarBgAnchor = mScene->GuiAnchors.Create(youtubeBarBg);
	youtubeBarBgAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	youtubeBarBgAnchor.Position = { 25.0f, 165.0f };

	mScene->SpriteRenderers.Get(youtubeBar).SpriteColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor = { 0.3f, 0.3f, 0.3f, 0.7f };

	{
		mScene->SpriteRenderers.Get(youtubeBar).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeHud).SpriteColor.a = 0.0f;
		mScene->SpriteRenderers.Get(youtubeFade).SpriteColor.a = 0.0f;
		mScene->TextRenderers.Get(youtubeTitle).Color.a = 0.0f;
		mScene->TextRenderers.Get(youtubeDuration).Color.a = 0.0f;

		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeBar).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeBarBg).SpriteColor.a, 0.7f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeHud).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->SpriteRenderers.Get(youtubeFade).SpriteColor.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->TextRenderers.Get(youtubeTitle).Color.a, 1.0f, 2.0f, Easing::Linear);
		pTimedActionSystem->PushAction(&mScene->TextRenderers.Get(youtubeDuration).Color.a, 1.0f, 2.0f, Easing::Linear);
	}

	mConductor->AddScriptedEvent(0, [this]()
		{
			startupVideo = mScene->EntityManager.CreateEntity();
			auto& sprite = mScene->SpriteRenderers.Create(startupVideo);
			auto& transform = mScene->Transforms.Create(startupVideo);

			auto& surface = mScene->VideoSurfaces.Create(startupVideo);
			surface.Path = "fnf/videos/GatoBros.mp4";
			mScene->InitVideo(surface);

			surface.SetPlayState(true);

			sprite.Rect.position = {};
			sprite.Rect.size = surface.VideoResolution;
			sprite.IsGui = true;
			sprite.RenderLayer = 101;
			sprite.TextureHandle = surface.TextureHandle;
			sprite.SpriteColor.r = 0.0f;
			sprite.SpriteColor.g = 0.0f;
			sprite.SpriteColor.b = 0.0f;
			transform.SetScale(glm::vec3(20.0f));

			gGameState.DoBeatEveryNthBeat = 999999999;

			auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
			auto& props = metadataManager->GetEntities();

			for (auto entity : props)
			{
				auto& metadata = metadataManager->Get(entity);
				auto& nameTag = mScene->Names.Get(entity);
				auto& transform = mScene->Transforms.Get(entity);
				auto& sprite = mScene->SpriteRenderers.Get(entity);

				if (nameTag.Name.starts_with("Muelto"))
				{
					metadata.Position.y += 1520.0f;
				}
			}

			CharaSprite* chara;
			CharaSprite* lastChara = mIngameSystem->GetOpponentCharacter();
			mIngameSystem->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("Syobon"));
			chara->CharaPosition = { -156.0f, 76.0f };
			chara->CharaScale = { 0.825f, 0.825f };
			mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
			chara->SetEnabled(true);
		});
	mConductor->AddScriptedEvent(257, [this]() {

		x64entity = mScene->EntityManager.CreateEntity();
		x64blackentity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(x64entity);
		auto& transform = mScene->Transforms.Create(x64entity);
		auto& sprite1 = mScene->SpriteRenderers.Create(x64blackentity);
		auto& transform1 = mScene->Transforms.Create(x64blackentity);

		mScene->EntityManager.DestroyEntity(startupVideo);
		startupVideo = Stratum::ECS::C_INVALID_ENTITY;

		sprite1.SpriteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		sprite1.Rect.size = { 100000, 100000 };
		sprite1.IsGui = true;
		sprite1.RenderLayer = 101;
		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen1.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = true;
		sprite.RenderLayer = 102;
		sprite.IsGui = true;
		transform.SetScale(glm::vec3(1.25f));

		});
	mConductor->AddScriptedEvent(272, [this]() {
		mScene->EntityManager.DestroyEntity(x64blackentity);
		mScene->EntityManager.DestroyEntity(x64entity);
		gGameState.DoBeatEveryNthBeat = 2;
		});
	mConductor->AddScriptedEvent(773, [this]() {
		CharaSprite* chara;
		CharaSprite* lastChara = mIngameSystem->GetOpponentCharacter();
		mIngameSystem->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("Syobon"));
		chara->CharaPosition = { -156.0f, 76.0f };
		chara->CharaScale = { 0.825f, 0.825f };
		lastChara->SetEnabled(true);
		});
	mConductor->AddScriptedEvent(1536, [this]() {
		CharaSprite* sprite;
		mIngameSystem->SetPlayerCharacter(sprite = CharaRegistry::GetCharacter("syobon"));
		sprite->CharaPosition = {};
		mScene->SpriteRenderers.Get(sprite->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(sprite->CharaEntity).IsGui = true;
		});
	mConductor->AddScriptedEvent(1550, [this]() {
		mConductor->EnableBot = true;
		Stratum::VarRegistry::GetConsoleVar("r", "post_ca_enabled")->set(true);
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1556, [this]() {
		mConductor->EnableBot = false;
		});
	mConductor->AddScriptedEvent(1174, [this]() {
		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Muelto"))
			{
				pTimedActionSystem->PushAction(&metadata.Position.y, metadata.Position.y - 1520.0f, mConductor->StepsToSeconds(3), Easing::Linear);
			}
		}
		});
	mConductor->AddScriptedEvent(1177, [this]() {
		CharaSprite* lastChara = mIngameSystem->GetOpponentCharacter();
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		lastChara->SetEnabled(true);
		lastChara->PlayAnimation("die");
		pTimedActionSystem->PushAction(&lastChara->CharaPosition.y, lastChara->CharaPosition.y - 2500.0f * 1.5f, 1.2f * 1.25f, Easing::BackIn);
		});
	mConductor->AddScriptedEvent(1200, [this]() {
		mIngameSystem->GetOpponentCharacter()->CharaPosition.y += 200;
		auto& sprite = mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity);
		sprite.Center = {};
		pTimedActionSystem->PushAction(&sprite.Rotation.x, 30, ActionParameters(0.9f, 28.0f), Easing::Sine);
		});
	mConductor->AddScriptedEvent(1218, [this]() {
		auto& sprite = mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, -3600, 1.0f, Easing::SineIn);
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaPosition.y, -3600, 1.0f, Easing::SineIn);
		});
	mConductor->AddScriptedEvent(1270, [this]() {
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("FernanBebe"));
		CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y += 1600;
		CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.x = -CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.x;
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y, CharaRegistry::GetCharacter("FernanBebe")->CharaPosition.y - 1600, 2.0f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(1356, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("uahh");
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x, CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x + 1.5f, 0.15f, Easing::Linear);
		});
	mConductor->AddScriptedEvent(1360, [this]() {
		CharaRegistry::GetCharacter("FernanBebe")->CharaScale.x -= 1.5f;
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("repeat");
		});
	mConductor->AddScriptedEvent(1379, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("nopodemos");
		});
	mConductor->AddScriptedEvent(1391, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("sipodemos");
		});
	mConductor->AddScriptedEvent(1400, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("queno");
		});
	mConductor->AddScriptedEvent(1407, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("sipodemos");
		});
	mConductor->AddScriptedEvent(1415, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("waa");
		});
	mConductor->AddScriptedEvent(1420, [this]() {
		auto manager = mScene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);
		auto entity = mScene->EntityManager.CreateEntity();
		auto& transform = mScene->Transforms.Create(entity);
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& animator = mScene->SpriteAnimators.Create(entity);
		manager->Create(entity);

		transform.SetPosition(glm::vec3(969.000, 609.000, 0.0f));

		auto frames = SparrowReader::readXML("fnf/SyobonNoAction/holasaul.xml", "holasaul", false);

		Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(30)
			.SetLoop(false)
			.SetNextState("destroy")
			.SetFrames(frames);

		animator.AnimationMap["saul"] = animation;
		animator.SetState("saul");

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/holasaul.png");
		sprite.RenderLayer = 10000;
		});
	mConductor->AddScriptedEvent(1536, [this]() {
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity).IsGui = true;
		auto& sprite = mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity);
		sprite.Center = { 0.0f, 1.0f };

		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Muelto"))
			{
				metadata.Position.y += 15200.0f;
			}
		}

		});
	mConductor->AddScriptedEvent(1547, [this]() {
		CharaRegistry::GetCharacter("Fernan")->PlayAnimation("WTF");
		});
	mConductor->AddScriptedEvent(1551, [this]() {
		mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity).RenderLayer = 10000;
		mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity).IsGui = false;
		});
	mConductor->AddScriptedEvent(2089, [this]() {
		CharaSprite* chara;
		mIngameSystem->SetOpponentCharacter(chara = CharaRegistry::GetCharacter("rene"));
		chara->CharaPosition = { -746.0f, -212.0f };
		chara->CharaScale = { 0.825f, 0.825f };
		Stratum::VarRegistry::GetConsoleVar("r", "post_ca_enabled")->set(false);
		});
	mConductor->AddScriptedEvent(2613, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->SetEnabled(true);
		CharaRegistry::GetCharacter("FernanRene")->SetLayer(1000);
		CharaRegistry::GetCharacter("FernanRene")->CharaPosition = { mScene->VirtualScreenSize.x + 9000.0f, 0.0f };
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("FernanRene")->CharaPosition, glm::vec2(0.0f), 0.13f, Easing::Linear);
		});
	mConductor->AddScriptedEvent(2615, [this]() {
		auto oponent = CharaRegistry::GetCharacter("rene");
		mScene->SpriteRenderers.Get(oponent->CharaEntity).SpriteColor = glm::vec4(1.5f, 0.0f, 0.0f, 1.0f);
		pTimedActionSystem->PushAction(&oponent->CharaPosition, glm::vec2(oponent->CharaPosition.x - mScene->VirtualScreenSize.x / 2.0f, oponent->CharaPosition.y), 0.2f, Easing::ElasticInOut,
			[this]() {
				auto oponent = CharaRegistry::GetCharacter("rene");
				auto& sprite = mScene->SpriteRenderers.Get(oponent->CharaEntity);
				mScene->SpriteRenderers.Get(oponent->CharaEntity).SpriteColor = glm::vec4(1.5f, 0.0f, 0.0f, 1.0f);
				pTimedActionSystem->PushAction(&sprite.Rotation.x, 360, 0.8f, Easing::Random);
				pTimedActionSystem->PushAction(&oponent->CharaScale, oponent->CharaScale + glm::vec2(0.75f, 0), 0.8f, Easing::Random,
					[this]() {
						auto oponent = CharaRegistry::GetCharacter("rene");
						auto& sprite = mScene->SpriteRenderers.Get(oponent->CharaEntity);
						pTimedActionSystem->PushAction(&oponent->CharaPosition, glm::vec2(oponent->CharaPosition.x - 1000.0f, oponent->CharaPosition.y), 0.4f, Easing::Random);
						pTimedActionSystem->PushAction(&sprite.Rotation.x, -360, 0.8f, Easing::Random);
					});
			});
		});
	mConductor->AddScriptedEvent(2648, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2676, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2678, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2680, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2682, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2684, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2698, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("nosabe");
		});
	mConductor->AddScriptedEvent(2700, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("eh");
		});
	mConductor->AddScriptedEvent(2704, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->PlayAnimation("nosabe");
		});
	mConductor->AddScriptedEvent(2720, [this]() {
		CharaRegistry::GetCharacter("FernanRene")->SetEnabled(false);
		x64entity = mScene->EntityManager.CreateEntity();
		x64blackentity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(x64entity);
		auto& transform = mScene->Transforms.Create(x64entity);
		auto& sprite1 = mScene->SpriteRenderers.Create(x64blackentity);
		auto& transform1 = mScene->Transforms.Create(x64blackentity);

		sprite1.SpriteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		sprite1.Rect.size = { 100000, 100000 };
		sprite1.IsGui = true;
		sprite1.RenderLayer = 101;
		sprite.TextureHandle = mScene->Resources.LoadTextureImage("fnf/SyobonNoAction/screen2.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = true;
		sprite.RenderLayer = 102;
		sprite.IsGui = true;
		transform.SetScale(glm::vec3(0.25f));
		});
	mConductor->AddScriptedEvent(2727, [this]() {
		mScene->EntityManager.DestroyEntity(x64blackentity);
		mScene->EntityManager.DestroyEntity(x64entity);
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("FernanJumpeado"));
		CharaRegistry::GetCharacter("FernanJumpeado")->CharaPosition.x = -CharaRegistry::GetCharacter("FernanJumpeado")->CharaPosition.x;
		});
	mConductor->AddScriptedEvent(2958, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("no");
		});
	mConductor->AddScriptedEvent(2970, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("nO");
		});
	mConductor->AddScriptedEvent(2976, [this]() {
		CharaRegistry::GetCharacter("FernanJumpeado")->PlayAnimation("NO");
		});
	mConductor->AddScriptedEvent(2982, [this]() {
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaScale, mIngameSystem->GetOpponentCharacter()->CharaScale + glm::vec2(2.5f, 0.0f), 0.2f, Easing::SineInOut
			, [this]()
			{
				mIngameSystem->GetOpponentCharacter()->SetEnabled(false);
			});
		});
	mConductor->AddScriptedEvent(3108, [this]() {
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		CharaRegistry::GetCharacter("Fernan")->CharaPosition.x -= 1000.0f;
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("Fernan")->CharaPosition.x, CharaRegistry::GetCharacter("Fernan")->CharaPosition.x + 1000.0f, 0.3f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(3239, [this]() {
		CharaRegistry::GetCharacter("Fernan")->PlayAnimation("ahh");
		pTimedActionSystem->PushAction(&CharaRegistry::GetCharacter("Fernan")->CharaPosition, glm::vec2(0.0f, -200), 0.5f, Easing::SineOut);
		});
	mConductor->AddScriptedEvent(3255, [this]() {
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));
		CharaRegistry::GetCharacter("Fernan")->SetEnabled(true);
		mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity).RenderLayer = 10000;
		});
	mConductor->AddScriptedEvent(3784, [this]() {
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaPosition, mIngameSystem->GetOpponentCharacter()->CharaPosition + glm::vec2(160, -160), 0.35f, Easing::SineOut,
			[this]() {
				pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaPosition, mIngameSystem->GetOpponentCharacter()->CharaPosition + glm::vec2(160, -160), 0.35f, Easing::SineOut);
			});
		});
	mConductor->AddScriptedEvent(3800, [this]() {
		mIngameSystem->GetOpponentCharacter()->CharaPosition.y += 200;
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaPosition, mIngameSystem->GetOpponentCharacter()->CharaPosition - glm::vec2(320, -320), 0.35f, Easing::SineOut);
		auto& sprite = mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, 30.0f, ActionParameters(1.3f, 30.0f), Easing::Sine);
		sprite.Center = {};
		});
	mConductor->AddScriptedEvent(3835, [this]() {
		auto& sprite = mScene->SpriteRenderers.Get(mIngameSystem->GetOpponentCharacter()->CharaEntity);
		pTimedActionSystem->PushAction(&sprite.Rotation.x, -20.0f, 1.7f * 4.0, Easing::SineInOut);
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaScale, mIngameSystem->GetOpponentCharacter()->CharaScale + glm::vec2(0.6f, 0.0f), 1.7f * 3.0f, Easing::SineInOut);
		pTimedActionSystem->PushAction(&mIngameSystem->GetOpponentCharacter()->CharaPosition, mIngameSystem->GetOpponentCharacter()->CharaPosition - glm::vec2(0, 1200 * 2.0f), 1.7f * 3.0, Easing::SineInOut);
		});
	mConductor->AddScriptedEvent(3856, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3857, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3858, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(3859, [this]() {
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("llorapues");
		});
	mConductor->AddScriptedEvent(4056, [this]() {
		pTimedActionSystem->ClearActions();
		StageRegistry::SetStage("syobon-end");
		mIngameSystem->SetPlayerCharacter(CharaRegistry::GetCharacter("Syobon"));
		mIngameSystem->SetOpponentCharacter(CharaRegistry::GetCharacter("Fernan"));

		mIngameSystem->GetOpponentCharacter()->SetCenter({ 0.0f, 1.0f });
		mIngameSystem->GetOpponentCharacter()->SetEnabled(false);

		mScene->SpriteRenderers.Get(mIngameSystem->GetPlayerCharacter()->CharaEntity).FlipX = false;
		});
	mConductor->AddScriptedEvent(4074, [this]() {
		auto metadataManager = mScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
		auto& props = metadataManager->GetEntities();

		for (auto entity : props)
		{
			auto& metadata = metadataManager->Get(entity);
			auto& nameTag = mScene->Names.Get(entity);
			auto& transform = mScene->Transforms.Get(entity);
			auto& sprite = mScene->SpriteRenderers.Get(entity);

			if (nameTag.Name.starts_with("Tb"))
			{
				pTimedActionSystem->PushAction(&metadata.Position.y, metadata.Position.y - 1000.0f, mConductor->StepsToSeconds(6), Easing::SineIn);
			}
		}

		CharaSprite* chara = mIngameSystem->GetPlayerCharacter();

		pTimedActionSystem->PushAction(&chara->CharaPosition.y, chara->CharaPosition.y - 1000.0f, mConductor->StepsToSeconds(6), Easing::SineIn);

		});
	mConductor->AddScriptedEvent(4082, [this]() {
		mIngameSystem->GetOpponentCharacter()->SetEnabled(true);
		mIngameSystem->GetOpponentCharacter()->PlayAnimation("llorapues");
		});
}

void Funkin::BiteFernanSong::Update()
{
	if (startupVideo != Stratum::ECS::C_INVALID_ENTITY)
	{
		auto& transform = mScene->Transforms.Get(startupVideo);
		auto& sprite = mScene->SpriteRenderers.Get(startupVideo);
		glm::vec2 scaleFactor = glm::vec2(mScene->VirtualScreenSize) / glm::vec2(sprite.Rect.size);
		transform.SetScale(glm::vec3(scaleFactor, 0.0f));

		sprite.SpriteColor.r += 0.1f * Stratum::gpGlobals->deltaTime;
		sprite.SpriteColor.g += 0.1f * Stratum::gpGlobals->deltaTime;
		sprite.SpriteColor.b += 0.1f * Stratum::gpGlobals->deltaTime;

		sprite.SpriteColor = glm::min(sprite.SpriteColor, glm::vec4(1.0f));
	}

	uint32_t seconds = mConductor->SongTime;

	// Scale sprites to screen size
	{
		auto& sprite = mScene->SpriteRenderers.Get(youtubeHud);
		glm::vec2 scaleFactor = glm::vec2(mScene->VirtualScreenSize) / glm::vec2(sprite.Rect.size + glm::ivec2(67, 30));
		mScene->Transforms.Get(youtubeHud).SetScale(glm::vec3(scaleFactor, 0.0f));
	}
	{
		auto& sprite = mScene->SpriteRenderers.Get(youtubeFade);
		glm::vec2 scaleFactor = glm::vec2(mScene->VirtualScreenSize) / glm::vec2(sprite.Rect.size);
		mScene->Transforms.Get(youtubeFade).SetScale(glm::vec3(scaleFactor, 0.0f));
	}
	{
		auto& sprite = mScene->SpriteRenderers.Get(youtubeBar);
		auto& sprite1 = mScene->SpriteRenderers.Get(youtubeBarBg);
		auto& transform = mScene->Transforms.Get(youtubeBar);
		transform.Scale.x = glm::mix(transform.Scale.x, (float)seconds / 291.0f, Stratum::Time::DeltaTime);
		transform.IsDirty = true;
		sprite.Rect.size.x = (mScene->VirtualScreenSize.x * 2.0f - 50.0f);
		sprite1.Rect.size.x = (mScene->VirtualScreenSize.x * 2.0f - 50.0f);
	}

	mScene->TextComponents.Get(youtubeDuration).Text = Stratum::Utils::FormatString(L"{}:{:02d} / 4:51", seconds / 60, seconds % 60);
}

void Funkin::BiteFernanSong::OnStep(int step)
{
}
