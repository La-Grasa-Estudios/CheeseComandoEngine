#include "EntityManager.h"

using namespace ENGINE_NAMESPACE;

ECS::EntityManager::EntityManager()
{
	memset(mValidEntities.data(), 0, sizeof(mValidEntities));
	MaxEntities = C_MAX_ENTITIES;
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
		LiveEntities += 1;
		return slot + 1;
	}
	return 0;
}

void ECS::EntityManager::DestroyEntity(edict_t entity)
{
	if (!IsValid(entity))
		return;
	// This needs to be here to delay the entity destruction until the end of the frame
	// because we might be using the iterator on some of the component managers and that
	// can cause a crash, bad.
	mRemovalsPending.push_back(entity);
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
	Update(); // Need to manually clear the removal queue
}

bool ECS::EntityManager::IsValid(edict_t entity)
{
	return entity != C_INVALID_ENTITY && mValidEntities[entity - 1];
}

void ECS::EntityManager::RegisterRemoval(const EntityRemovalEvent& func)
{
	mRemovals.push_back(func);
}

void ECS::EntityManager::Update()
{
	// Delete all entities that are pending 
	for (auto entity : mRemovalsPending)
	{
		for (size_t i = 0; i < mRemovals.size(); i++)
		{
			mRemovals[i](entity);
		}
		LiveEntities -= 1;
		mValidEntities[entity - 1] = false;
		mSearchStart = std::min(mSearchStart, entity - 1);
	}
	mRemovalsPending.clear();
}
