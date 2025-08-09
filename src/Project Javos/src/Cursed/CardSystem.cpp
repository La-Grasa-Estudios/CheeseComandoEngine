#include "CardSystem.h"

#include <Core/Time.h>

#include "Components.h"

template<typename T>
static inline T clampMix(T a, T b, float t)
{
	return glm::mix(a, b, glm::clamp(t, 0.0f, 1.0f));
}

Funkin::CardSystem::CardSystem() : mScene()
{
}

Funkin::CardSystem::~CardSystem()
{
}

void Funkin::CardSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;
}


static inline float easeInBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return c3 * x * x * x - c1 * x * x;
}



void Funkin::CardSystem::Update(Stratum::Scene* scene)
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

	bool anyCardGrabbed = false;

	for (auto entity : entities)
	{
		float fps = 1.0f / Stratum::Time::UnscaledDeltaTime;

		auto& card = cardManager->Get(entity);

		if (card.grabbed)
			anyCardGrabbed = true;
	}

	for (auto entity : entities)
	{
		float fps = 1.0f / Stratum::Time::UnscaledDeltaTime;

		auto& card = cardManager->Get(entity);
		auto& transform1 = mScene->Transforms.Get(entity);
		auto& transform2 = mScene->Transforms.Get(card.bgEntity);
		auto& transform3 = mScene->Transforms.Get(card.bgShadowEntity);

		auto& sprite1 = mScene->SpriteRenderers.Get(entity);
		auto& sprite2 = mScene->SpriteRenderers.Get(card.bgEntity);
		auto& sprite3 = mScene->SpriteRenderers.Get(card.bgShadowEntity);

		sprite1.RenderLayer = card.renderLayer + 2;
		sprite2.RenderLayer = card.renderLayer + 1;
		sprite3.RenderLayer = card.renderLayer + 0;

		float rotation = glm::sin(Stratum::Time::GlobalTime + card.seed) * 2.0f;
		float tiltX = glm::sin(Stratum::Time::GlobalTime + card.seed) * card.tiltFactor;
		float tiltY = glm::cos(Stratum::Time::GlobalTime + card.seed) * card.tiltFactor;
		float offset = glm::cos(Stratum::Time::GlobalTime + card.seed) * 4.0f;

		if (card.grabbed)
		{
			tiltX = 0.0f;
			tiltY = 0.0f;
			rotation = 0.0f;
			offset = 0.0f;
		}
		else
		{
			rotation += card.userRotation;
		}

		glm::vec3 position = card.position + glm::vec3(0.0f, offset, 0.0f);
		glm::vec3 scale = transform1.Scale;

		auto lastPos = transform1.Position;

		transform1.SetPosition(clampMix(transform1.Position, position, Stratum::Time::UnscaledDeltaTime * card.moveSpeed));
		transform2.SetPosition(transform1.Position);
		transform3.Position = transform1.Position;

		auto newPos = transform1.Position;

		float mult = fps / 120.0f;

		rotation -= (newPos.x - lastPos.x) * mult;
		tiltX += (newPos.y - lastPos.y) * mult;

		card.rotation = clampMix(card.rotation, rotation, Stratum::Time::UnscaledDeltaTime * 32.0f);

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

		tiltX = 0.0f;
		tiltY = 0.0f;

		if (cardAABB.PointInside(mScene->VirtualMousePosition) && !anyCardGrabbed && card.isHovered)
		{
			if (!card.grabbed)
			{
				tiltX = (mScene->VirtualMousePosition.y - transform1.Position.y) / sprite1.Rect.size.y * 20.0f;
				tiltY = (transform1.Position.x - mScene->VirtualMousePosition.x) / sprite1.Rect.size.x * 20.0f;
			}
		}

		if (!card.grabbed)
		{
			card.moveSpeed = 4.0f;
		}

		card.tiltX = clampMix(card.tiltX, tiltX, Stratum::Time::DeltaTime * 16.0f);
		card.tiltY = clampMix(card.tiltY, tiltY, Stratum::Time::DeltaTime * 16.0f);

		if (card.grabbed)
		{
			card.moveSpeed = 200.0f;
			scale = clampMix(scale, glm::vec3(1.65f), Stratum::Time::DeltaTime * 100.0f);
			card.grabTimer += Stratum::Time::DeltaTime * 8.0f;
		}
		else
		{
			card.moveSpeed = 4.0f;
			scale = clampMix(scale, glm::vec3(1.55f), Stratum::Time::DeltaTime * 8.0f);
			card.grabTimer = 0.0f;
		}

		card.grabTimer = glm::clamp(card.grabTimer, 0.0f, 1.0f);

		scale *= 1.0f + glm::sin(card.grabTimer * glm::pi<float>() * 12.0f) * 0.02f;

		transform1.SetScale(scale * card.scaleFactor);
		transform2.SetScale(transform1.Scale);

	}

	float tiltX = 0.0f;
	float tiltY = 0.0f;

}

void Funkin::CardSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Funkin::CardSystem::RenderImGui(Stratum::Scene* scene)
{

}
