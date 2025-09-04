#include "BalatroSystem.h"

#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>
#include <Core/Time.h>
#include <Core/Logger.h>
#include <Input/Input.h>
#include <Font/FontRegistry.h>
#include <Util/Globals.h>
#include <Event/EventBus.h>
#include <Sound/AudioEngine.h>
 
#include "CardSystem.h"

#undef min
#undef max

struct BalatroFrameData
{
	float time;
	float dissolve;
};

template<typename T>
static inline T clampMix(T a, T b, float t)
{
	return glm::mix(a, b, glm::clamp(t, 0.0f, 1.0f));
}

Funkin::BalatroSystem::BalatroSystem() : 
	mBgEntity(Stratum::ECS::C_INVALID_ENTITY),
	mCardEntity(Stratum::ECS::C_INVALID_ENTITY),
	mExitButton(Stratum::ECS::C_INVALID_ENTITY),
	mExitText(Stratum::ECS::C_INVALID_ENTITY),
	mLogoEntity(Stratum::ECS::C_INVALID_ENTITY),
	mScene(nullptr),
	pTimedActionSystem(nullptr)
{
}

void Funkin::BalatroSystem::Init(Stratum::Scene* scene)
{
	mCanDissolve = true;
	mScene = scene;

	mScene->RegisterCustomSystem(new CardSystem());

	scene->FontRegistry.LoadFont("balatro", "fonts/m6x11plus.ttf");
	scene->RegisterCustomSystem(pTimedActionSystem = new TimedActionSystem());

	auto playingCardManager = new Stratum::ECS::ComponentManager<PlayingCardComponent>();
	auto cardManager = new Stratum::ECS::ComponentManager<CardComponent>();
	auto textManager = new Stratum::ECS::ComponentManager<TextTiltComponent>();
	mScene->RegisterCustomComponent(playingCardManager, C_PLAY_CARD_COMPONENT);
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
	sprite.CameraLayer = 0;
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
		sprite.CameraLayer = 0;
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
	mBalatroChips = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/chips1.mp3", mScene->AudioEngine->GetEngine());
	mBalatroCrumple = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/card.mp3", mScene->AudioEngine->GetEngine());
	mBalatroPick = Stratum::CreateRef<Stratum::MP3AudioSource>("balatro/pick.mp3", mScene->AudioEngine->GetEngine());
	mBalatroSoundtrack = Stratum::CreateRef<Stratum::SngAudioSource>("balatro/music1.sng", mScene->AudioEngine->GetEngine());
	mScene->AudioEngine->AddSource(mBalatroSoundtrack);
	mScene->AudioEngine->AddSource(mBalatroWhoosh);
	mScene->AudioEngine->AddSource(mBalatroMagic);
	mScene->AudioEngine->AddSource(mBalatroCrumple);
	mScene->AudioEngine->AddSource(mBalatroPick);
	mScene->AudioEngine->AddSource(mBalatroChips);
	mBalatroSoundtrack->Play();
	mBalatroSoundtrack->SetPitch(0.70f);
	mBalatroSoundtrack->SetVolume(0.5f);
	mBalatroWhoosh->SetVolume(0.35f);
	mBalatroMagic->SetVolume(0.45f);
	mBalatroCrumple->SetVolume(0.45f);
	mBalatroPick->SetVolume(0.45f);

	mExitText = CreateTextEntity(L"Exit", { 0.0f, -2000.0f }, 100.0f, true, 10000, 0.5f);

	mPokerHandText = CreateTextEntity(L"HAND HERE", {}, 100.0f, true, 10000, 0.5f);
	auto& pokerHandAnchor = mScene->GuiAnchors.Create(mPokerHandText);
	pokerHandAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;
	pokerHandAnchor.Position = { 0.0f, 200.0f };

	mChipsText = CreateTextEntity(L"CHIPS HERE", {}, 100.0f, true, 10000, 0.0f);
	auto& chipsTextAnchor = mScene->GuiAnchors.Create(mChipsText);
	chipsTextAnchor.AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM_LEFT;
	chipsTextAnchor.Position = { 100.0f, 200.0f };

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

	for (int i = 0; i < 6; i++)
	{
		CreatePlayingCard((CardType)(12 - i), CARD_SUIT_HEARTS);
	}
	for (int i = 0; i < 4; i++)
	{
		CreatePlayingCard(CARD_TYPE_KING, (CardSuit)i);
	}
	for (int i = 0; i < 2; i++)
	{
		CreatePlayingCard(CARD_TYPE_JACK, (CardSuit)i);
	}

	SortCards();
}

void Funkin::BalatroSystem::Update(Stratum::Scene* scene)
{
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);

	if (!mPlayedMenuIntro)
	{
		mPlayedMenuIntro = true;
		mBalatroMagic->Play();
		pTimedActionSystem->PushAction(&mDissolveTime, 0.0f, 2.5f, Easing::Linear, [cardManager, this]
			{
				auto& sprite = mScene->SpriteRenderers.Get(mLogoEntity);
				sprite.pCustomShader = NULL;
				mCardEntity = CreateCard(CARD_TYPE_KING, CARD_SUIT_HEARTS);

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

				pTimedActionSystem->PushAction(&mDissolveTime, 0.0f, 0.8f, Easing::Linear, [this]
					{
						mPlayMenu = true;
					});

			});
	}

	mScene->RenderPath2D->SetConstantBuffer(mPerFrameData.get(), 2);

	auto& sprite = mScene->SpriteRenderers.Get(mBgEntity);
	sprite.Rect.size = scene->VirtualScreenSize;

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
		pTimedActionSystem->PushAction(&exitTextTransform.Position.y, -700.0f, 0.3f, Easing::BackOut);
		pTimedActionSystem->PushAction(&exitTransform.Position.y, -700.0f, 0.3f, Easing::BackOut);
	}

	auto& transform = mScene->Transforms.Get(mLogoEntity);

	auto& card = cardManager->Get(mCardEntity);

	if (!card.grabbed)
		card.position = transform.Position;

	//mNextCardTimer += Stratum::gpGlobals->deltaTime;

	if (mNextCardTimer > 5.0f && !mCurrentGrab && mCanDissolve)
	{
		mNextCardTimer = 0.0f;
		DissolveCard(mCardEntity);
	}

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
		if (Stratum::Input::GetMouseButtonDown(0))
			Stratum::EventBus::InvokeEvent(Stratum::ApplicationEvent{Stratum::ApplicationEvent::APP_EVENT_SHUTDOWN});

		if (!enter)
		{
			enter = true;
			timer = glm::pi<float>();
		}

		original *= 1.1f;
	}
	else
	{
		enter = false;
	}

	timer -= Stratum::gpGlobals->deltaTime * glm::pi<float>() * 5.0f;
	if (timer < 0.0f)
		timer = 0.0f;

	exitTransform.SetScale(original + glm::vec3(0.05f) * glm::sin(timer * 4.0f));

	const float gameSpeed = 4.0f;

	if (mIsPlayingHand)
	{
		auto playingCardManager = mScene->GetComponentManager<PlayingCardComponent>(C_PLAY_CARD_COMPONENT);
		auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);
		auto& entities = playingCardManager->GetEntities();

		if (!mEvents.empty())
		{
			auto& event = mEvents[0];

			if (mProcessNextEvent)
			{
				event.Duration /= gameSpeed;
				if (event.Type == EVENT_DRAW_CARD)
				{
					auto& playingCard = playingCardManager->Get(event.Entity);
					auto& card = cardManager->Get(event.Entity);
					card.position.y += 400.0f;
					const glm::vec2 handRect = { 142 * 2.1f * mPlayedCardsCount / 2.0f, -300.0f };
					float cardMargin = (handRect.x * 2.0f) / mPlayedCardsCount;
					card.position.x = -handRect.x + (event.drawCardIndex + 0.5f) * cardMargin;
					mBalatroPick->Play();
				}
				if (event.Type == EVENT_SCORE_CARD)
				{
					auto& playingCard = playingCardManager->Get(event.Entity);
					auto& card = cardManager->Get(event.Entity);

					pTimedActionSystem->PushAction(&card.scaleFactor, 0.02f, { event.Duration, 9.0f }, Easing::SineAdd);
					mBalatroChips->SetPitch(event.SoundPitch);
					mBalatroChips->Play();
				}
				if (event.Type == EVENT_END_SCORING)
				{
					mEvents.clear();
					mProcessNextEvent = true;
					mIsPlayingHand = false;
					return;
				}
				mProcessNextEvent = false;
			}

			event.Duration -= Stratum::Time::DeltaTime;

			if (event.Duration <= 0.0f)
			{
				mEvents.erase(mEvents.begin());
				mProcessNextEvent = true;
			}
		}
		return;
	}

	mProcessNextEvent = true;

	UpdateCards();

	if (Stratum::Input::GetKeyDown(KeyCode::S))
	{
		SortCards();
	}

	if (Stratum::Input::GetKeyDown(KeyCode::P))
	{
		SolvePokerHandType(true);
	}

	SolvePokerHandType();
}

void Funkin::BalatroSystem::PostUpdate(Stratum::Scene* scene)
{
	if (mBalatroBgShader->ShaderDesc.RenderTarget != mScene->RenderPath2D->GetRenderTarget())
	{
		mBalatroBgShader->SetRenderTarget(mScene->RenderPath2D->GetRenderTarget());
		mBalatroDissolveShader->SetRenderTarget(mScene->RenderPath2D->GetRenderTarget());
	}

	mCmdBuffer->Begin();

	BalatroFrameData frameData{};

	frameData.time = Stratum::Time::GlobalTime;
	frameData.dissolve = mDissolveTime;

	mCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), &frameData);

	mCmdBuffer->End();
	mCmdBuffer->Submit();
}

void Funkin::BalatroSystem::RenderImGui(Stratum::Scene* scene)
{

}

void Funkin::BalatroSystem::UpdateCards()
{
	struct CardInstance
	{
		Stratum::ECS::edict_t entity;

		union
		{
			uint64_t sort;
			CardSuit suit;
			CardType type;
		};

		int32_t cardPos;

		constexpr bool operator >(const CardInstance& other) const
		{
			return sort > other.sort || cardPos > other.cardPos;
		}

		constexpr bool operator <(const CardInstance& other) const
		{
			return sort < other.sort || cardPos < other.cardPos;
		}

	};

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
	auto playingCardManager = mScene->GetComponentManager<PlayingCardComponent>(C_PLAY_CARD_COMPONENT);
	auto& entities = cardManager->GetEntities();

	static glm::vec3 grabOffset = {};
	static glm::vec3 grabPosition = {};
	static Stratum::ECS::edict_t grab = Stratum::ECS::C_INVALID_ENTITY;

	std::vector<CardInstance> overEntities;
	const uint32_t maxSelectedCards = 5;
	uint32_t selectedCardAmt = 0;

	for (auto entity : entities)
	{
		float fps = 1.0f / Stratum::Time::UnscaledDeltaTime;

		auto& card = cardManager->Get(entity);

		auto& transform1 = mScene->Transforms.Get(entity);
		auto& sprite1 = mScene->SpriteRenderers.Get(entity);

		AABB cardAABB = {
			transform1.Position.x - sprite1.Rect.size.x * transform1.Scale.x,
			transform1.Position.y - sprite1.Rect.size.y * transform1.Scale.y,
			transform1.Position.x + sprite1.Rect.size.x * transform1.Scale.x,
			transform1.Position.y + sprite1.Rect.size.y * transform1.Scale.y
		};

		if (cardAABB.PointInside(mScene->VirtualMousePosition))
		{
			CardInstance c;
			c.sort = card.renderLayer;
			c.entity = entity;

			overEntities.push_back(c);
		}

		card.grabbed = (grab == entity);
		card.isHovered = false;

		if (card.grabbed)
		{
			card.grabbedTimer += Stratum::Time::DeltaTime;
		}
		else
		{
			card.grabbedTimer = 0;
		}

		if (playingCardManager->HasComponent(entity))
		{
			auto& playingCard = playingCardManager->Get(entity);
			if (playingCard.selected)
			{
				selectedCardAmt++;
			}
		}

	}

	std::sort(overEntities.begin(), overEntities.end(), std::greater<CardInstance>());

	if (!overEntities.empty()) 
	{
		auto entity = overEntities[0].entity;
		auto& card = cardManager->Get(entity);

		auto& transform1 = mScene->Transforms.Get(entity);
		auto& sprite1 = mScene->SpriteRenderers.Get(entity);

		if (Stratum::Input::GetMouseButtonDown(0))
		{
			grabOffset = transform1.Position - glm::vec3(mScene->VirtualMousePosition, 0.0f);
			grabPosition = transform1.Position;
			grab = entity;
			mBalatroPick->Play();
		}
		if (!Stratum::Input::GetMouseButton(0) && grab != Stratum::ECS::C_INVALID_ENTITY)
		{
			auto& card1 = cardManager->Get(grab);
			if (playingCardManager->HasComponent(grab) && card1.grabbedTimer < 0.1f)
			{
				auto& playingCard = playingCardManager->Get(grab);
				bool newSelected = !playingCard.selected;

				if (newSelected && selectedCardAmt >= maxSelectedCards)
				{
					newSelected = false;
				}

				playingCard.selected = newSelected;
			}
			grab = Stratum::ECS::C_INVALID_ENTITY;
		}
		
		card.isHovered = true;
		card.grabbed = (grab == entity);
	}

	float tiltX = 0.0f;
	float tiltY = 0.0f;

	if (grab != Stratum::ECS::C_INVALID_ENTITY)
	{
		auto& card = cardManager->Get(grab);
		if (card.grabbedTimer > 0.1f)
			card.position = glm::vec3(mScene->VirtualMousePosition, 0.0f) + grabOffset;
	}

	mCurrentGrab = grab;

	auto& cards = playingCardManager->GetEntities();

	const glm::vec2 handRect = { 142 * 1.5f * 10.0f / 2.0f, -300.0f };

	std::vector<CardInstance> sortedCards;
	bool cardGrabbed = false;

	for (auto entity : cards)
	{
		auto& card = cardManager->Get(entity);
		auto& playingCard = playingCardManager->Get(entity);

		CardInstance i;
		i.cardPos = card.position.x;
		i.entity = entity;

		sortedCards.push_back(i);

		if (card.grabbed)
		{
			cardGrabbed = true;
		}
	}

	if (cardGrabbed)
	{
		std::sort(sortedCards.begin(), sortedCards.end(), std::less<CardInstance>());

		uint32_t i = 0;
		for (auto& c : sortedCards)
		{
			auto& card = cardManager->Get(c.entity);
			auto& playingCard = playingCardManager->Get(c.entity);

			playingCard.cardIndex = i;

			i++;
		}
	}

	float cardMargin = (handRect.x * 2.0f) / sortedCards.size();

	for (auto& c : cards)
	{
		auto& card = cardManager->Get(c);
		auto& playingCard = playingCardManager->Get(c);

		card.renderLayer = playingCard.cardIndex * 3 + 10;
		if (!card.grabbed)
		{
			card.position.x = -handRect.x + (playingCard.cardIndex + 0.5f) * cardMargin;
			if (playingCard.selected)
			{
				card.position.y = clampMix(card.position.y, handRect.y + 100.0f, Stratum::Time::DeltaTime * 32.0f);
			}
			else
			{
				card.position.y = clampMix(card.position.y, handRect.y, Stratum::Time::DeltaTime * 32.0f);
			}
		}
	}
}

void Funkin::BalatroSystem::SortCards()
{
	struct CardInstance
	{
		Stratum::ECS::edict_t entity;

		union
		{
			uint64_t sort;
			struct
			{
				uint8_t type;
				CardSuit suit;
			} bits;
		};

		constexpr bool operator >(const CardInstance& other) const
		{
			return sort > other.sort;
		}

		constexpr bool operator <(const CardInstance& other) const
		{
			return sort < other.sort;
		}

	};

	auto playingCardManager = mScene->GetComponentManager<PlayingCardComponent>(C_PLAY_CARD_COMPONENT);
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);

	auto& cards = playingCardManager->GetEntities();

	std::vector<CardInstance> sortedCards;

	for (auto entity : cards)
	{
		auto& card = cardManager->Get(entity);
		auto& playingCard = playingCardManager->Get(entity);

		CardInstance i{};
		i.bits.type = CardTypeSordWeight[playingCard.type];
		i.bits.suit = playingCard.suit;
		i.entity = entity;

		sortedCards.push_back(i);
	}

	std::sort(sortedCards.begin(), sortedCards.end(), std::greater<CardInstance>());


	uint32_t i = 0;
	for (auto& c : sortedCards)
	{
		auto& card = cardManager->Get(c.entity);
		auto& playingCard = playingCardManager->Get(c.entity);

		playingCard.cardIndex = i;

		i++;
	}
}

void Funkin::BalatroSystem::SolvePokerHandType(bool playCards)
{
	struct CardInstance
	{
		Stratum::ECS::edict_t entity;

		uint8_t sort;

		constexpr bool operator >(const CardInstance& other) const
		{
			return sort > other.sort;
		}

		constexpr bool operator <(const CardInstance& other) const
		{
			return sort < other.sort;
		}

	};

	if (mIsPlayingHand)
		return;
	std::vector<CardInstance> selectedCards;
	std::array<uint32_t, 13> cardCount{};
	std::array<uint32_t, 4> suitCount{};
	std::array<std::array<uint32_t, 13>, 4> cardsPerSuitCount{};
	uint32_t cardsSelected = 0;

	auto playingCardManager = mScene->GetComponentManager<PlayingCardComponent>(C_PLAY_CARD_COMPONENT);
	auto cardManager = mScene->GetComponentManager<CardComponent>(C_CARD_COMPONENT);

	auto& cards = playingCardManager->GetEntities();

	for (auto entity : cards)
	{
		auto& playingCard = playingCardManager->Get(entity);
		auto& card = cardManager->Get(entity);

		if (playingCard.selected)
		{
			suitCount[playingCard.suit] += 1;
			cardCount[playingCard.type] += 1;
			cardsPerSuitCount[playingCard.suit][playingCard.type] += 1;
			cardsSelected++;
			selectedCards.push_back({ entity, playingCard.cardIndex });
		}
	}

	std::sort(selectedCards.begin(), selectedCards.end(), std::less<CardInstance>());

	bool isHighCard = cardsSelected;
	bool isFlush = suitCount[0] == 5 || suitCount[1] == 5 || suitCount[2] == 5 || suitCount[3] == 5;
	bool isRoyal = false;
	bool isStraight = false;
	bool isStraightFlush = false;
	bool isThreeOfAKind = false;
	bool isFourOfAKind = false;
	bool isFiveOfAKind = false;
	bool isPair = false;
	bool isTwoPair = false;
	bool isFlushHouse = false;
	bool isFlushFive = false;

	CardType cardTypeThreeOfAKind = CARD_TYPE_INVALID;
	CardType cardTypeFourOfAKind = CARD_TYPE_INVALID;
	CardType cardTypePair1 = CARD_TYPE_INVALID;
	CardType cardTypePair2 = CARD_TYPE_INVALID;
	CardType cardTypeHighest = CARD_TYPE_INVALID;

	uint32_t consecutiveCardsCount = 0;

	for (int i = 0; i < 13; i++)
	{
		if (cardCount[i] != 0)
		{
			cardTypeHighest = (CardType)i;
		}
		if (cardCount[i] == 5)
		{
			isFiveOfAKind = true;
		}
		if (cardCount[i] == 4)
		{
			isFourOfAKind = true;
			cardTypeFourOfAKind = (CardType)i;
		}
		if (cardCount[i] == 3)
		{
			isThreeOfAKind = true;
			cardTypeThreeOfAKind = (CardType)i;
		}
		if (cardCount[i] == 2)
		{
			isPair = true;
			cardTypePair1 = (CardType)i;
			for (int j = 0; j < 13; j++)
			{
				if (i != j && cardCount[j] == 2)
				{
					isTwoPair = true;
					cardTypePair2 = (CardType)j;
					break;
				}
			}
		}
		if (cardCount[i] > 0)
		{
			consecutiveCardsCount++;
			if (consecutiveCardsCount >= 5)
			{
				isStraight = true;
			}
		}
		else
		{
			consecutiveCardsCount = 0;
		}
	}

	if (isFlush)
	{
		for (int k = 0; k < 4; k++)
		{
			bool containsPair = false;
			bool containsThree = false;
			uint32_t count = 0;
			for (int i = 0; i < 13; i++)
			{
				if (cardsPerSuitCount[k][i] == 5)
				{
					isFlushFive = true;
					break;
				}
				if (cardsPerSuitCount[k][i] == 2)
				{
					containsPair = true;
				}
				if (cardsPerSuitCount[k][i] == 3)
				{
					containsThree = true;
				}
				if (containsPair && containsThree)
				{
					isFlushHouse = true;
					break;
				}
			}
			
			for (int i = 8; i < 13; i++)
			{
				count += (bool)cardsPerSuitCount[k][i] ? 1 : 0;
			}
			if (count == 5)
			{
				isRoyal = true;
				break;
			}
			uint32_t consecutiveCount = 0;
			for (int i = 0; i < 13; i++)
			{
				if (cardsPerSuitCount[k][i] > 0)
				{
					consecutiveCount++;
					if (consecutiveCount >= 5)
					{
						isStraightFlush = true;
						break;
					}
				}
				else
				{
					consecutiveCount = 0;
				}
			}
		}
	}

	bool isFullHouse = isThreeOfAKind && isPair;

	auto& text = mScene->TextComponents.Get(mPokerHandText);

	text.Text = L"";

	if (isHighCard) text.Text = L"High Card";
	if (isPair) text.Text = L"Pair";
	if (isTwoPair) text.Text = L"Two Pair";
	if (isThreeOfAKind) text.Text = L"Three of a kind";
	if (isStraight) text.Text = L"Straight";
	if (isFlush) text.Text = L"Flush";
	if (isFullHouse) text.Text = L"Full House";
	if (isFourOfAKind) text.Text = L"Four of a kind";
	if (isStraightFlush) text.Text = L"Straight Flush";
	if (isRoyal) text.Text = L"Royal Flush";
	if (isFiveOfAKind) text.Text = L"Five of a kind";
	if (isFlushHouse) text.Text = L"Flush House";
	if (isFlushFive) text.Text = L"Flush Five";

	bool doesHandPlayFivecards = isFlush || isFullHouse || isStraight || isRoyal || isFlushHouse || isFlushFive;

	if (playCards)
	{
		mEvents.clear();
		mIsPlayingHand = true;

		uint32_t cardIndex = 0;

		std::vector<GameEvent> scoreEvents;

		for (auto c : selectedCards)
		{
			auto& playingCard = playingCardManager->Get(c.entity);
			auto& card = cardManager->Get(c.entity);
			playingCard.selected = false;

			GameEvent event{};

			event.Type = EVENT_DRAW_CARD;
			event.Entity = c.entity;
			event.Duration = 0.5f;
			event.drawCardIndex = cardIndex;
			mEvents.push_back(event);
			cardIndex++;

			if (!doesHandPlayFivecards)
			{
				if (isFourOfAKind)
				{
					if (playingCard.type != cardTypeFourOfAKind)
					{
						continue;
					}
				}
				else if (isThreeOfAKind)
				{
					if (playingCard.type != cardTypeThreeOfAKind)
					{
						continue;
					}
				}
				else if (isTwoPair)
				{
					if (playingCard.type != cardTypePair1 && playingCard.type != cardTypePair2)
					{
						continue;
					}
				}
				else if (isPair)
				{
					if (playingCard.type != cardTypePair1)
					{
						continue;
					}
				}
				else if (isHighCard)
				{
					if (playingCard.type != cardTypeHighest)
					{
						continue;
					}
				}
			}

			event.Duration = 1.0f;
			event.Type = EVENT_SCORE_CARD;
			event.SoundPitch = 0.75f + cardIndex * 0.1f;
			scoreEvents.push_back(event);
		}

		GameEvent wait{};
		wait.Type = EVENT_WAIT;
		wait.Duration = 1.0f;

		mEvents.push_back(wait);

		for (auto e : scoreEvents)
		{
			mEvents.push_back(e);
		}

		mEvents.push_back(wait);

		GameEvent event{};
		event.Type = EVENT_END_SCORING;
		mEvents.push_back(event);

		mPlayedCardsCount = selectedCards.size();
	}

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

	pTimedActionSystem->PushAction(&sprite.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);
	pTimedActionSystem->PushAction(&sprite1.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);
	pTimedActionSystem->PushAction(&sprite2.Rotation.x, 4.0f, { 0.5f, 4 }, Easing::Sine);
	 
	pTimedActionSystem->PushAction(&mDissolveTime, 1.0f, 0.5f, Easing::Linear, [&, cardEntity]
		{
			static int lastCard = 0;
			auto cardX = 11 + rand() % 2;
			auto cardY = rand() % 4;

			while (cardY == lastCard)
			{
				cardY = rand() % 4;
			}

			lastCard = cardY;

			DestroyCard(cardEntity);
			mCardEntity = CreateCard((CardType)cardX, (CardSuit)cardY);

			pTimedActionSystem->PushAction(&mDissolveTime, 0.0f, 0.8f, Easing::Linear, [&, cardEntity]
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

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateCard(CardType type, CardSuit suit)
{
	uint32_t cardX = type;
	uint32_t cardY = suit;

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
		sprite.CameraLayer = 0;
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
		sprite.CameraLayer = 0;
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
		sprite.CameraLayer = 0;
		sprite.FlipX = false;
		sprite.Center = { 0.0f, 0.0f };
		sprite.pCustomShader = mBalatroDissolveShader.get();
		transform.SetScale(glm::vec3(1.55f));
	}

	card.tiltFactor = 16.0f;

	return cardEntity;
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreatePlayingCard(CardType type, CardSuit suit)
{
	auto entity = CreateCard(type, suit);

	auto playingCardManager = mScene->GetComponentManager<PlayingCardComponent>(C_PLAY_CARD_COMPONENT);
	auto& playingCard = playingCardManager->Create(entity);

	playingCard.chips = BaseCardChips[type];
	playingCard.suit = suit;
	playingCard.type = type;

	return entity;
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize, uint8_t cameraLayer, uint32_t renderLayer, float align)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto textManager = mScene->GetComponentManager<TextTiltComponent>(C_TILT_COMPONENT);
	mScene->TextComponents.Create(entity);
	mScene->TextComponents.Get(entity).FontSize = fontSize;
	mScene->TextComponents.Get(entity).Text = defaultText;
	mScene->TextComponents.Get(entity).Font = "balatro";
	mScene->TextRenderers.Create(entity).Alignment = align;
	mScene->TextRenderers.Get(entity).RenderLayer = renderLayer;
	mScene->TextRenderers.Get(entity).CameraLayer = cameraLayer;
	mScene->Transforms.Create(entity);
	mScene->Transforms.Get(entity).SetPosition(glm::vec3(pos, 0.0f));
	textManager->Create(entity).seed = rand();
	textManager->Get(entity).credits = cameraLayer == 0;
	return entity;
}

Stratum::ECS::edict_t Funkin::BalatroSystem::CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize, const glm::vec2& center, uint8_t cameraLayer, uint32_t renderLayer)
{
	auto entity = mScene->EntityManager.CreateEntity();
	auto& sprite = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);

	sprite.Rect.size = rectSize;
	sprite.UseNearestTextureFilter = false;
	sprite.RenderLayer = renderLayer;
	sprite.CameraLayer = cameraLayer;
	sprite.Center = center;

	transform.SetPosition(glm::vec3(pos, 1.0f));

	return entity;
}

Funkin::BalatroSystem::~BalatroSystem()
{
}