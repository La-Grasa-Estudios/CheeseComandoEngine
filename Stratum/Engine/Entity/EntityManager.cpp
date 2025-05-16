#include "EntityManager.h"

using namespace ENGINE_NAMESPACE;

ECS::EntityManager::EntityManager()
{
	memset(mValidEntities.data(), 0, sizeof(mValidEntities));
}

ECS::edict_t ECS::EntityManager::CreateEntity()
{
	int32_t slot = -1;
	for (uint32_t i = mSearchStart; i < C_MAX_ENTITIES; i++)
	{
		if (!mValidEntities[i])
		{
			slot = i;
			mSearchStart = i + 1;
			break;
		}
	}
	if (slot != -1)
	{
		mValidEntities[slot] = true;
		return slot + 1;
	}
	return 0;
}

void ECS::EntityManager::DestroyEntity(edict_t entity)
{
	if (!IsValid(entity))
		return;
	for (size_t i = 0; i < mRemovals.size(); i++)
	{
		mRemovals[i](entity);
	}
	mValidEntities[entity - 1] = false;
	mSearchStart = std::min(mSearchStart, entity - 1);
}

void ECS::EntityManager::DestroyAll()
{
	for (edict_t i = 0; i < C_MAX_ENTITIES; i++)
	{
		if (mValidEntities[i])
		{
			DestroyEntity(i+1);
		}
	}
}

bool ECS::EntityManager::IsValid(edict_t entity)
{
	return entity != C_INVALID_ENTITY && mValidEntities[entity - 1];
}

void ECS::EntityManager::RegisterRemoval(const EntityRemovalEvent& func)
{
	mRemovals.push_back(func);
}
