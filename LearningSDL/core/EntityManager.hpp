#pragma once
#ifndef ENTITYMANAGER_HPP
#define ENTITYMANAGER_HPP

#include "Entity.hpp"
#include <iostream>
#include <queue>
#include "Components.hpp"
#include <array>

class EntityManager
{
public:
	EntityManager();
	// ~EntityManager();

	Entity CreateEntity();

	void DestroyEntity(Entity entity);
	void SetSignature(Entity entity, Signature signature);
	Signature GetSignature(Entity entity);

private:
	std::queue<Entity> mAvailableEntities;
	std::array<Signature, MAX_ENTITIES> mSignatures;
	uint64_t mLivingEntitiesCount;
};

#endif //!ENTITYMANAGER_HPP
