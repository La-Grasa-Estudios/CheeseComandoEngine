#include "CardSystem.h"

#include <Entity/ComponentManager.h>

static const char* CardSprites[] = {
	"cris_k.png",
	"javi_k.png",
	"mono_j.png",
	"pedro_q.png"
};

void CardSystem::Init(Stratum::Scene* scene)
{
	scene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<Card>(), "Card");
	scene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<InHandCard>(), "InHand");
	m_Scene = scene;
}

void CardSystem::Update(Stratum::Scene* scene)
{
	auto inHandManager = scene->GetComponentManager<InHandCard>("InHand");
	auto count = inHandManager->GetEntities().size();
	float halfPoint = (float)count / 2.0f;
	float handWidth = 1200.0f;
	float startX = -handWidth / 2.0f;
	for (auto entity : inHandManager->GetEntities())
	{
		auto& transform = scene->Transforms.Get(entity);
		auto& inHandComp = inHandManager->Get(entity);

		float handSpacing = handWidth / ((float)(count - 1) + 0.0001f);
		float x = startX + inHandComp.SlotIndex * handSpacing;
		float radius = handWidth;
		float distToCenter = glm::clamp(glm::abs(x), 0.0f, radius * glm::pi<float>());
		float halfCircle = glm::cos(distToCenter / radius);
		float neg = glm::sign(x);

		float angle = neg * halfCircle * glm::pi<float>();

		float y = halfCircle * radius - radius;
		y += -m_Scene->VirtualScreenSize.y + 400.0f;
		
		transform.Rotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
		transform.Position = glm::vec3(x, y, 0.0f);
		transform.IsDirty = true;
	}
}

void CardSystem::PostUpdate(Stratum::Scene* scene)
{

}

void CardSystem::RenderImGui(Stratum::Scene* scene)
{

}

Stratum::ECS::edict_t CardSystem::CreateCard(CardType type)
{
	auto backgroundEntity = m_Scene->EntityManager.CreateEntity();
	auto entity = m_Scene->EntityManager.CreateEntity();

	auto& card = m_Scene->GetComponentManager<Card>("Card")->Create(entity);
	card.Type = type;

	auto& transform = m_Scene->Transforms.Create(entity);

	auto& sprite = m_Scene->SpriteRenderers.Create(entity);
	sprite.TextureHandle = m_Scene->Resources.LoadTextureImage(std::format("kernel/card/{}", CardSprites[static_cast<int>(type)]));
	sprite.UseNearestTextureFilter = true;
	sprite.Rect.size = m_Scene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();

	auto& backgroundSprite = m_Scene->SpriteRenderers.Create(backgroundEntity);
	m_Scene->Transforms.Create(backgroundEntity).Parent = entity;
	backgroundSprite.TextureHandle = m_Scene->Resources.LoadTextureImage("kernel/card/front.png");
	backgroundSprite.UseNearestTextureFilter = true;
	backgroundSprite.Rect.size = m_Scene->Resources.GetImageHandle(backgroundSprite.TextureHandle)->GetSize();

	sprite.RenderLayer = 100;
	backgroundSprite.RenderLayer = 90;

	return entity;
}

bool CardSystem::IsCardInHand(Stratum::ECS::edict_t cardEntity)
{
	return m_Scene->GetComponentManager<InHandCard>("InHand")->HasComponent(cardEntity);
}

void CardSystem::SetCardInHand(Stratum::ECS::edict_t cardEntity, bool inHand, uint32_t slotIndex)
{
	auto inHandManager = m_Scene->GetComponentManager<InHandCard>("InHand");
	if (inHand)
	{
		if (!inHandManager->HasComponent(cardEntity))
		{
			auto& inHandComp = inHandManager->Create(cardEntity);
			inHandComp.SlotIndex = slotIndex;
		}
		else
		{
			auto& inHandComp = inHandManager->Get(cardEntity);
			inHandComp.SlotIndex = slotIndex;
		}
	}
	else
	{
		if (inHandManager->HasComponent(cardEntity))
		{
			inHandManager->Remove(cardEntity);
		}
	}
}
