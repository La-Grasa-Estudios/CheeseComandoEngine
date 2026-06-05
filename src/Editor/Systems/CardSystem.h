#pragma once

#include <Scene/Scene.h>

#include <bitset>

enum class GameplayModifier
{
	GENERATOR,
	MASOCHIST,
	WANDERING,
	KAMIKAZE,
	UDPATE,
	CLONE,
	RELAY,
	CLOCK,
	PERSISTENT,
	THORNS,
	REINFORCEMENT,
	DIRECT,
	SHIELD,
	DOUBLE_TAP,
	RAGE,
	GLITCH,
	BUFFER,
	NERF,
	TRAP,
	MANUAL,
	HUNTER,
	COWARD,
	GOLEM,
	MONEY,
	CANCELLER,
	RECONFIGURE
};

enum class CardType
{
	CRIS,
	JAVI,
	MONO,
	PIEDRO
};

struct CardStats
{

};

struct Card
{
	CardType Type = CardType::CRIS;
	std::bitset<32> ModifierFlags = 0;
};

struct InHandCard
{
	uint32_t SlotIndex = 0;
};

class CardSystem : public Stratum::ISceneSystem
{
public:
	void Init(Stratum::Scene* scene) final;
	void Update(Stratum::Scene* scene) final;
	void PostUpdate(Stratum::Scene* scene) final;
	void RenderImGui(Stratum::Scene* scene) final;
	Stratum::ECS::edict_t CreateCard(CardType type);
	bool IsCardInHand(Stratum::ECS::edict_t cardEntity);
	void SetCardInHand(Stratum::ECS::edict_t cardEntity, bool inHand, uint32_t slotIndex = 0);
private:
	Stratum::Scene* m_Scene;
};