#include "BalatroSystem.h"

#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>
#include <Core/Time.h>
#include <Core/Logger.h>
#include <Input/Input.h>
#include <Font/FontRegistry.h>
#include <Util/Globals.h>
#include <Event/EventHandler.h>

#include <Sound/AudioEngine.h>
#undef min
#undef max

struct BalatroFrameData
{
	float time;
	float dissolve;
};

const float PI = glm::pi<float>();

static inline float easeInElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0.0f
		? 0
		: x == 1.0f
		? 1
		: -glm::pow(2, 10 * x - 10) * glm::sin((x * 10 - 10.75) * c4);
}

static inline float easeOutElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: glm::pow(2, -10 * x) * glm::sin((x * 10 - 0.75) * c4) + 1;
}

static inline float easeInOutElastic(float x) {
	float c5 = (2 * PI) / 4.5;

	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5
		? -(glm::pow(2, 20 * x - 10) * glm::sin((20 * x - 11.125) * c5)) / 2
		: (glm::pow(2, -20 * x + 10) * glm::sin((20 * x - 11.125) * c5)) / 2 + 1;
}

static inline float easeInBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return c3 * x * x * x - c1 * x * x;
}

static inline float easeOutBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return 1 + c3 * glm::pow(x - 1, 3) + c1 * glm::pow(x - 1, 2);
}

static inline float easeInOutBack(float x) {
	float c1 = 1.70158;
	float c2 = c1 * 1.525;

	return x < 0.5
		? (glm::pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
		: (glm::pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
}

Funkin::BalatroSystem::BalatroSystem()
{
	
}

void Funkin::BalatroSystem::Init(Stratum::Scene* scene)
{
	mCanDissolve = true;
	mScene = scene;

	scene->FontRegistry.LoadFont("balatro", "fonts/m6x11plus.ttf");

	auto cardManager = new Stratum::ECS::ComponentManager<CardComponent>();
	auto textManager = new Stratum::ECS::ComponentManager<TextTiltComponent>();
	mScene->RegisterCustomComponent(cardManager, C_CARD_COMPONENT);
	mScene->RegisterCustomComponent(textManager, C_TILT_COMPONENT);

	Stratum::Render::PipelineDescription pipelineDesc{};

	pipelineDesc.ShaderPath = "shaders/2d/balatro_bg.cso";
	pipelineDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(uint32_t)));

	pipelineDesc.VertexLayout.VertexAttributes.push_back({ Stratum::Render::VertexType::FLOAT2_32, Stratum::Render::VertexInputRate::PER_VERTEX, 0, 0, 0, false });
	pipelineDesc.VertexLayout.Stride = sizeof(glm::vec2);

	pipelineDesc.RasterizerState.CullMode = Stratum::Render::RCullMode::NOT;
	pipelineDesc.RasterizerState.DepthTest = false;
	pipelineDesc.StencilState.DepthEnable = false;

	pipelineDesc.BlendState.BlendStates[0].DestBlend = Stratum::Render::Blend::INV_SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].SrcBlend = Stratum::Render::Blend::SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].DestBlendAlpha = Stratum::Render::Blend::ONE;
	pipelineDesc.BlendState.BlendStates[0].SrcBlendAlpha = Stratum::Render::Blend::SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].EnableBlend = true;

	mBalatroBgShader = Stratum::CreateRef<Stratum::Render::GraphicsPipeline>(pipelineDesc);

	pipelineDesc.ShaderPath = "shaders/2d/balatro_dissolve.cso";

	mBalatroDissolveShader = Stratum::CreateRef<Stratum::Render::GraphicsPipeline>(pipelineDesc);

	mPerFrameData = Stratum::CreateRef<Stratum::Render::ConstantBuffer>(sizeof(BalatroFrameData));
	mCmdBuffer = Stratum::CreateRef<Stratum::Render::GraphicsCommandBuffer>();

	mBgEntity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(mBgEntity);
	auto& transform = mScene->Transforms.Create(mBgEntity);

	sprite.Rect.size = {};
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = 0;
	sprite.IsGui = false;
	sprite.Center = {};
	sprite.pCustomShader = mBalatroBgShader.get();
	
	{
		auto entity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& transform = mScene->Transforms.Create(entity);

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("balatro/balatro.png");
		sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		sprite.UseNearestTextureFilter = false;
		sprite.RenderLayer = 0;
		sprite.IsGui = false;
		sprite.FlipX = false;
		sprite.Center = { 0.0f, 0.0f };
		sprite.pCustomShader = mBalatroDissolveShader.get();

		transform.SetScale(glm::vec3(1.7f));
		transform.SetPosition({ 0.0f, 250.0f, 0.0f });

		mLogoEntity = entity;
	}

	mDissolveTime = 1.0f;

	mBalatroWhoosh = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/whoosh.mp3", mScene->AudioEngine->GetEngine());
	mBalatroMagic = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/crumple.mp3", mScene->AudioEngine->GetEngine());
	mBalatroCrumple = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/card.mp3", mScene->AudioEngine->GetEngine());
	mBalatroPick = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/pick.mp3", mScene->AudioEngine->GetEngine());
	mBalatroSoundtrack = Stratum::CreateRef<Stratum::SngAudioSource>("balatro/music1.sng", mScene->AudioEngine->GetEngine());
	mScene->AudioEngine->AddSource(mBalatroSoundtrack);
	mScene->AudioEngine->AddSource(mBalatroWhoosh);
	mScene->AudioEngine->AddSource(mBalatroMagic);
	mScene->AudioEngine->AddSource(mBalatroCrumple);
	mScene->AudioEngine->AddSource(mBalatroPick);
	mBalatroSoundtrack->Play();
	mBalatroSoundtrack->SetPitch(0.70f);
	mBalatroSoundtrack->SetVolume(0.5f);
	mBalatroWhoosh->SetVolume(0.35f);
	mBalatroMagic->SetVolume(0.45f);
	mBalatroCrumple->SetVolume(0.45f);
	mBalatroPick->SetVolume(0.45f);

	mExitText = CreateTextEntity(L"Exit", { 0.0f, -2000.0f }, 100.0f, true, 10000, 0.5f);

	auto exitButton = mExitButton = CreateRectEntity({ 0.0f, -2000.0f }, { 500, 100 }, { 0.0f, 0.0f }, true, 9000);
	textManager->Create(exitButton).credits = false;
	textManager->Get(mExitText).seed = 0;

	auto& exitSprite = mScene->SpriteRenderers.Get(exitButton);
	auto& exitTransform = mScene->Transforms.Get(exitButton);
	exitSprite.TextureHandle = mScene->Resources.LoadTextureImage("balatro/Enhancers.png");
	exitSprite.Rect.size = { 142, 190 };
	exitSprite.Rect.position = { 142 * 1, 190 * 0 };
	exitSprite.Rotation.x = 90.0f;
	exitSprite.SpriteColor = { 0.0f, 0.5f, 1.0f, 1.0f };
	exitTransform.SetScale(glm::vec3(1.2f, 0.8f, 1.0f));
}

void Funkin::BalatroSystem::Update(Stratum::Scene* scene)
{
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);

	if (!mPlayedMenuIntro)
	{
		mPlayedMenuIntro = true;
		mBalatroMagic->Play();
		PushAction(&mDissolveTime, 0.0f, 2.5f, Easing::Linear, [&]
			{
				auto& sprite = mScene->SpriteRenderers.Get(mLogoEntity);
				sprite.pCustomShader = NULL;
				mCardEntity = CreateCard(11, 0);

				auto& transform = mScene->Transforms.Get(mLogoEntity);
				auto& card = cardManager->Get(mCardEntity);
				auto& cardTransform = mScene->Transforms.Get(mCardEntity);
				card.position = transform.Position;
				cardTransform.SetPosition(card.position);

				auto& transform1 = mScene->Transforms.Get(mCardEntity);
				auto& transform2 = mScene->Transforms.Get(card.bgEntity);
				auto& transform3 = mScene->Transforms.Get(card.bgShadowEntity);

				auto& sprite1 = mScene->SpriteRenderers.Get(mCardEntity);
				auto& sprite2 = mScene->SpriteRenderers.Get(card.bgEntity);
				auto& sprite3 = mScene->SpriteRenderers.Get(card.bgShadowEntity);

				transform2.SetPosition(transform1.Position);
				transform3.Position = transform1.Position;

				mDissolveTime = 1.0f;
				mBalatroCrumple->Play();

				PushAction(&mDissolveTime, 0.0f, 0.8f, Easing::Linear, [&]
					{
						mPlayMenu = true;
					});

			});
	}

	for (int i = 0; i < mActions.size(); i++)
	{
		Action& act = mActions[i];

		if (act.time == 0.0f)
		{
			for (int i = 0; i < act.count; i++)
			{
				act.srcFloat[i] = act.floatPtr[i];
			}
		}

		act.time += Stratum::gpGlobals->deltaTime;

		float interp = glm::clamp(act.time / act.params.duration, 0.0f, 1.0f);

		for (int i = 0; i < act.count; i++)
		{
			float val = 0.0f;

			switch (act.easing)
			{
			case Easing::Linear:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], interp);
				break;
			case Easing::Random:
			{
				float randomVal = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::clamp(interp + randomVal, 0.0f, 1.0f));
			}
			break;
			case Easing::SineIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], 1 - glm::cos((interp * PI) / 2));
				break;
			case Easing::SineOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::sin((interp * PI) / 2.0f));
				break;
			case Easing::SineInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], -(glm::cos(PI * interp) - 1) / 2.0f);
				break;
			case Easing::ElasticIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInElastic(interp));
				break;
			case Easing::ElasticOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeOutElastic(interp));
				break;
			case Easing::ElasticInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInOutElastic(interp));
				break;
			case Easing::BackIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInBack(interp));
				break;
			case Easing::BackOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeOutBack(interp));
				break;
			case Easing::BackInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInOutBack(interp));
				break;
			case Easing::Sine:
				interp = glm::sin((interp * PI) / 2.0f);
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::sin(interp * (PI * 2) * act.params.amplitude));
				break;
			case Easing::Cosine:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::cos(interp * (PI * 2) * act.params.amplitude));
				break;
			default:
				break;
			}

			act.floatPtr[i] = val;
		}

		if (act.time >= act.params.duration)
		{
			if (act.cb)
			{
				act.cb();
			}
			mActions.erase(mActions.begin() + i);
			i--;
			continue;
		}
	}

	mScene->RenderPath3D->RenderPath2D->SetConstantBuffer(mPerFrameData.get(), 2);

	auto& sprite = mScene->SpriteRenderers.Get(mBgEntity);
	sprite.Rect.size = scene->VirtualScreenSize;

	mCmdBuffer->Begin();

	BalatroFrameData frameData{};

	frameData.time = Stratum::Time::GlobalTime;
	frameData.dissolve = mDissolveTime;

	mCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), &frameData);

	mCmdBuffer->End();
	mCmdBuffer->Submit();

	if (!mPlayMenu)
		return;

	auto& exitTextTransform = mScene->Transforms.Get(mExitText);
	auto& exitTransform = mScene->Transforms.Get(mExitButton);

	exitTextTransform.IsDirty = true;
	exitTransform.IsDirty = true;

	if (!mFirstMenuFrame)
	{
		mFirstMenuFrame = true;

		CreateTextEntity(L"Credits", { 0.0f, 0.0f }, 130.0f, false, 10, 0.5f);
		CreateTextEntity(L"Art by: Mono Pistola", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Game Idea: EasyDLC's", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Programming", { 0.0f, 0.0f }, 110.0f, false, 10, 0.5f);
		CreateTextEntity(L"Lead Programmer: CheeseCommando", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Game Programmer: CheeseCommando", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Engine Programmer: CheeseCommando", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Lead Engine Programmer: CheeseCommando", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Credits Balatro Menu: CheeseCommando", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Assets", { 0.0f, 0.0f }, 110.0f, false, 10, 0.5f);
		CreateTextEntity(L"LocalThunk (Credits)", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"FranaGonzala (Cover)", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Pelosdelechuga (Original Concept)", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"Josno_json (Original Playable game)", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
		CreateTextEntity(L"camboi1234 (Original Chart)", { 0.0f, 0.0f }, 90.0f, false, 10, 0.5f);
	}

	auto textManager = mScene->GetComponentManager<TextTiltComponent>(C_TILT_COMPONENT);
	auto& textEntities = textManager->GetEntities();

	static float creditsTime = 0.0f;

	creditsTime += mScene->VirtualScreenSize.y * Stratum::Time::DeltaTime * 0.2f;

	uint32_t index = 0;
	float minY = 42340.0f;

	for (auto entity : textEntities)
	{
		auto& tilt = textManager->Get(entity);
		float rotation = glm::sin(Stratum::Time::GlobalTime + tilt.seed) * 2.0f;
		float tiltX = glm::sin(Stratum::Time::GlobalTime + tilt.seed) * 20.0f;
		float tiltY = glm::cos(Stratum::Time::GlobalTime + tilt.seed) * 20.0f;
		float offset = glm::cos(Stratum::Time::GlobalTime + tilt.seed) * 5.0f;

		auto& transform1 = mScene->Transforms.Get(entity);
		if (tilt.credits)
		{
			transform1.SetPosition(glm::vec3(0.0f, offset + creditsTime - mScene->VirtualScreenSize.y - 100.0f - index * 500.0f, 0.0f));
			minY = glm::min(minY, transform1.Position.y);
			index++;
		}

		transform1.SetRotation(glm::vec3(glm::radians(tiltX), glm::radians(tiltY), glm::radians(rotation)));
	}

	if (!mFirstButtonFrame && minY > mScene->VirtualScreenSize.y)
	{
		mFirstButtonFrame = true;
		PushAction(&exitTextTransform.Position.y, -700.0f, 0.3f, Easing::BackOut);
		PushAction(&exitTransform.Position.y, -700.0f, 0.3f, Easing::BackOut);
	}

	auto& transform = mScene->Transforms.Get(mLogoEntity);

	auto& card = cardManager->Get(mCardEntity);

	if (!card.grabbed)
		card.position = transform.Position;

	mNextCardTimer += Stratum::gpGlobals->deltaTime;

	if (mNextCardTimer > 5.0f && !mCurrentGrab && mCanDissolve)
	{
		mNextCardTimer = 0.0f;
		DissolveCard(mCardEntity);
	}

	UpdateCards();

	struct AABB
	{
		float x0;
		float y0;
		float x1;
		float y1;
		bool Overlap(const AABB& other) const
		{
			return other.x1 > x0 && other.x0 < x1 && other.y1 > y0 && other.y0;
		}
		bool PointInside(glm::vec2 point) const
		{
			return point.x > x0 && point.x < x1 && point.y > y0 && point.y < y1;
		}
	};

	auto& sprite1 = mScene->SpriteRenderers.Get(mExitButton);

	AABB cardAABB = {
			exitTransform.Position.x - sprite1.Rect.size.x * exitTransform.Scale.x,
			exitTransform.Position.y - sprite1.Rect.size.y * exitTransform.Scale.y,
			exitTransform.Position.x + sprite1.Rect.size.x * exitTransform.Scale.x,
			exitTransform.Position.y + sprite1.Rect.size.y * exitTransform.Scale.y
	};

	glm::vec3 original = { 1.2f, 0.8f, 1.0f };

	static float timer = 0.0f;
	static bool enter = false;

	if (cardAABB.PointInside(mScene->VirtualMousePosition))
	{
		if (Stratum::Input::GetMouseButttonDown(0))
			Stratum::EventHandler::InvokeEvent(Stratum::EventHandler::GetEventID("app_close"), this);

		if (!enter)
		{
			enter = true;
			timer = PI;
		}

		original *= 1.1f;
	}
	else
	{
		enter = false;
	}

	timer -= Stratum::gpGlobals->deltaTime * PI * 5.0f;
	if (timer < 0.0f)
		timer = 0.0f;

	exitTransform.SetScale(original + glm::vec3(0.05f) * glm::sin(timer * 4.0f));
}

void Funkin::BalatroSystem::PostUpdate(Stratum::Scene* scene)
{
	if (mBalatroBgShader->ShaderDesc.RenderTarget != mScene->RenderPath3D->RenderPath2D->GetRenderTarget())
	{
		mBalatroBgShader->SetRenderTarget(mScene->RenderPath3D->RenderPath2D->GetRenderTarget());
		mBalatroDissolveShader->SetRenderTarget(mScene->RenderPath3D->RenderPath2D->GetRenderTarget());
	}
}

void Funkin::BalatroSystem::RenderImGui(Stratum::Scene* scene)
{

}

void Funkin::BalatroSystem::UpdateCards()
{
	struct AABB
	{
		float x0;
		float y0;
		float x1;
		float y1;
		bool Overlap(const AABB& other) const
		{
			return other.x1 > x0 && other.x0 < x1 && other.y1 > y0 && other.y0;
		}
		bool PointInside(glm::vec2 point) const
		{
			return point.x > x0 && point.x < x1 && point.y > y0 && point.y < y1;
		}
	};

	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);
	auto& entities = cardManager->GetEntities();

	static glm::vec3 grabOffset = {};
	static Stratum::ECS::edict_t grab = Stratum::ECS::C_INVALID_ENTITY;


	for (auto entity : entities)
	{
		auto& card = cardManager->Get(entity);
		auto& transform1 = mScene->Transforms.Get(entity);
		auto& transform2 = mScene->Transforms.Get(card.bgEntity);
		auto& transform3 = mScene->Transforms.Get(card.bgShadowEntity);

		auto& sprite1 = mScene->SpriteRenderers.Get(entity);
		auto& sprite2 = mScene->SpriteRenderers.Get(card.bgEntity);
		auto& sprite3 = mScene->SpriteRenderers.Get(card.bgShadowEntity);

		float rotation = glm::sin(Stratum::Time::GlobalTime + card.seed) * 2.0f;
		float tiltX = glm::sin(Stratum::Time::GlobalTime + card.seed) * card.tiltFactor;
		float tiltY = glm::cos(Stratum::Time::GlobalTime + card.seed) * card.tiltFactor;
		float offset = glm::cos(Stratum::Time::GlobalTime + card.seed) * 4.0f;

		glm::vec3 position = card.position + glm::vec3(0.0f, offset, 0.0f);
		glm::vec3 scale = transform1.Scale;

		auto lastPos = transform1.Position;

		transform1.SetPosition(glm::mix(transform1.Position, position, Stratum::Time::UnscaledDeltaTime * card.moveSpeed));
		transform2.SetPosition(transform1.Position);
		transform3.Position = transform1.Position;

		auto newPos = transform1.Position;

		rotation -= (newPos.x - lastPos.x);
		tiltX += (newPos.y - lastPos.y);

		card.rotation = glm::mix(card.rotation, rotation, Stratum::Time::UnscaledDeltaTime * 32.0f);

		transform1.SetRotation(glm::vec3(glm::radians(tiltX + card.tiltX), glm::radians(tiltY + card.tiltY), glm::radians(card.rotation)));
		transform2.SetRotation(transform1.Rotation);
		transform3.Rotation = transform1.Rotation;

		transform3.ModelMatrix = glm::translate(transform1.ModelMatrix, glm::vec3(9.0f, -9.0f, 0.0f));

		AABB cardAABB = {
			transform1.Position.x - sprite1.Rect.size.x * transform1.Scale.x,
			transform1.Position.y - sprite1.Rect.size.y * transform1.Scale.y,
			transform1.Position.x + sprite1.Rect.size.x * transform1.Scale.x,
			transform1.Position.y + sprite1.Rect.size.y * transform1.Scale.y
		};
		
		card.grabbed = (grab == entity);

		tiltX = 0.0f;
		tiltY = 0.0f;

		if (cardAABB.PointInside(mScene->VirtualMousePosition))
		{
			if (Stratum::Input::GetMouseButttonDown(0))
			{
				grabOffset = transform1.Position - glm::vec3(mScene->VirtualMousePosition, 0.0f);
				grab = entity;
				mBalatroPick->Play();
			}
			if (!Stratum::Input::GetMouseButton(0))
				grab = Stratum::ECS::C_INVALID_ENTITY;

			if (!card.grabbed)
			{
				tiltX = (mScene->VirtualMousePosition.y - transform1.Position.y) / sprite1.Rect.size.y * 24.0f;
				tiltY = (transform1.Position.x - mScene->VirtualMousePosition.x) / sprite1.Rect.size.x * 24.0f;
			}
		}
		else
		{
			if (grab == entity)
				grab = Stratum::ECS::C_INVALID_ENTITY;
			card.moveSpeed = 4.0f;
		}

		card.tiltX = glm::mix(card.tiltX, tiltX, Stratum::Time::DeltaTime * 8.0f);
		card.tiltY = glm::mix(card.tiltY, tiltY, Stratum::Time::DeltaTime * 8.0f);

		if (card.grabbed)
		{
			card.moveSpeed = 200.0f;
			scale = glm::mix(scale, glm::vec3(1.60f), Stratum::Time::DeltaTime * 100.0f);
		}
		else
		{
			card.moveSpeed = 4.0f;
			scale = glm::mix(scale, glm::vec3(1.55f), Stratum::Time::DeltaTime * 8.0f);
		}

		transform1.SetScale(scale);
		transform2.SetScale(transform1.Scale);

	}

	float tiltX = 0.0f;
	float tiltY = 0.0f;

	if (grab != Stratum::ECS::C_INVALID_ENTITY)
	{
		auto& card = cardManager->Get(grab);
		card.position = glm::vec3(mScene->VirtualMousePosition, 0.0f) + grabOffset;
	}

	mCurrentGrab = grab;
}

void Funkin::BalatroSystem::DissolveCard(Stratum::ECS::edict_t cardEntity)
{
	if (!mCanDissolve)
		return;

	mBalatroWhoosh->Play();

	mCanDissolve = false;

	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);
	auto& card = cardManager->Get(cardEntity);
	auto& sprite = mScene->SpriteRenderers.Get(cardEntity);
	auto& sprite1 = mScene->SpriteRenderers.Get(card.bgEntity);
	auto& sprite2 = mScene->SpriteRenderers.Get(card.bgShadowEntity);

	PushAction(&sprite.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);
	PushAction(&sprite1.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);
	PushAction(&sprite2.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);

	PushAction(&mDissolveTime, 1.0f, 0.5f, Easing::Linear, [&, cardEntity]
		{
			static int lastCard = -1;
			auto cardX = 11 + rand() % 2;
			auto cardY = rand() % 4;

			while (cardY == lastCard)
			{
				cardY = rand() % 4;
			}

			lastCard = cardY;

			DestroyCard(cardEntity);
			mCardEntity = CreateCard(cardX, cardY);

			PushAction(&mDissolveTime, 0.0f, 0.8f, Easing::Linear, [&, cardEntity]
				{
					mCanDissolve = true;
				});
		});
}

void Funkin::BalatroSystem::DestroyCard(Stratum::ECS::edict_t cardEntity)
{
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);
	auto& card = cardManager->Get(cardEntity);

	mScene->EntityManager.DestroyEntity(card.bgEntity);
	mScene->EntityManager.DestroyEntity(card.bgShadowEntity);
	mScene->EntityManager.DestroyEntity(cardEntity);
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateCard(uint32_t cardX, uint32_t cardY)
{
	uint32_t enhancementX = 1;
	uint32_t enhancementY = 0;

	auto entity1 = mScene->EntityManager.CreateEntity();
	auto entity = mScene->EntityManager.CreateEntity();
	auto cardEntity = mScene->EntityManager.CreateEntity();
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);

	auto& card = cardManager->Create(cardEntity);

	card.seed = rand();

	{
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& transform = mScene->Transforms.Create(entity);

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("balatro/Enhancers.png");
		sprite.Rect.size = { 142, 190 };
		sprite.Rect.position = { 142 * enhancementX, 190 * enhancementY };
		sprite.UseNearestTextureFilter = false;
		sprite.RenderLayer = 1;
		sprite.IsGui = false;
		sprite.FlipX = false;
		sprite.Center = { 0.0f, 0.0f };
		sprite.pCustomShader = mBalatroDissolveShader.get();
		transform.SetScale(glm::vec3(1.55f));

		card.bgEntity = entity;
	}
	{
		auto& sprite = mScene->SpriteRenderers.Create(entity1);
		auto& transform = mScene->Transforms.Create(entity1);

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("balatro/Enhancers.png");
		sprite.Rect.size = { 142, 190 };
		sprite.Rect.position = { 142 * enhancementX, 190 * enhancementY };
		sprite.UseNearestTextureFilter = false;
		sprite.RenderLayer = 0;
		sprite.IsGui = false;
		sprite.FlipX = false;
		sprite.Center = { 0.0f, 0.0f };
		sprite.SpriteColor = { 0.0f, 0.0f, 0.0f, 0.5f };
		sprite.pCustomShader = mBalatroDissolveShader.get();
		transform.SetScale(glm::vec3(1.55f));

		card.bgShadowEntity = entity1;
	}
	{
		auto& sprite = mScene->SpriteRenderers.Create(cardEntity);
		auto& transform = mScene->Transforms.Create(cardEntity);

		sprite.TextureHandle = mScene->Resources.LoadTextureImage("balatro/8BitDeck.png");
		sprite.Rect.size = { 142, 190 };
		sprite.Rect.position = { 142 * cardX, 190 * cardY };
		sprite.UseNearestTextureFilter = false;
		sprite.RenderLayer = 1;
		sprite.IsGui = false;
		sprite.FlipX = false;
		sprite.Center = { 0.0f, 0.0f };
		sprite.pCustomShader = mBalatroDissolveShader.get();
		transform.SetScale(glm::vec3(1.55f));
	}

	card.tiltFactor = 16.0f;

	return cardEntity;
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize, bool isGui, uint32_t renderLayer, float align)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto textManager = mScene->GetComponentManager<TextTiltComponent>(C_TILT_COMPONENT);
	mScene->TextComponents.Create(entity);
	mScene->TextComponents.Get(entity).FontSize = fontSize;
	mScene->TextComponents.Get(entity).Text = defaultText;
	mScene->TextComponents.Get(entity).Font = "balatro";
	mScene->TextRenderers.Create(entity).Alignment = align;
	mScene->TextRenderers.Get(entity).RenderLayer = renderLayer;
	mScene->TextRenderers.Get(entity).IsGui = isGui;
	mScene->Transforms.Create(entity);
	mScene->Transforms.Get(entity).SetPosition(glm::vec3(pos, 0.0f));
	textManager->Create(entity).seed = rand();
	textManager->Get(entity).credits = !isGui;
	return entity;
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize, const glm::vec2& center, bool isGui, uint32_t renderLayer)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.Rect.size = rectSize;
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.IsGui = isGui;
	sprite.Center = center;

	transform.SetPosition(glm::vec3(pos, 1.0f));

	return entity;
}


// Some hot shit incoming, but it does the job

void Funkin::BalatroSystem::PushAction(float* dst, float targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 1;
	action.params = params;
	action.easing = easing;
	action.floatPtr = dst;
	action.targetFloat[0] = targetVal;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 2;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 3;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 4;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.targetFloat[3] = targetVal.w;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(float* dst, float targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 1;
	action.params = params;
	action.easing = easing;
	action.floatPtr = dst;
	action.targetFloat[0] = targetVal;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 2;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 3;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::BalatroSystem::PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 4;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.targetFloat[3] = targetVal.w;
	action.cb = cb;
	mActions.push_back(action);
}

// End of hot shit

Funkin::BalatroSystem::ActionParameters::ActionParameters(const float dur) : duration(dur)
{
}

Funkin::BalatroSystem::ActionParameters::ActionParameters(const float dur, const float amp) : duration(dur), amplitude(amp)
{
}

Funkin::BalatroSystem::~BalatroSystem()
{
}
