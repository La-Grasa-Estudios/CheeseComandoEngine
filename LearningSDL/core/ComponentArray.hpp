#pragma once
#ifndef COMPONENTARRAY_HPP
#define COMPONENTARRAY_HPP
#include <unordered_map>
#include "Entity.hpp"
//for every component there is a separate array
class IComponentArray
{
public:
	virtual ~IComponentArray() = default;
	virtual void DestroyEntity(Entity entity) = 0;
};

template <typename T>
class ComponentArray : public IComponentArray
{
public:
	void InsertData(Entity entity, T component)
	{
		if (mEntitytoIndexMap.find(entity) != mEntitytoIndexMap.end())
		{
			std::cout << "\nTrying to add a duplicate component for the same entity. Ensure each entity has only one of this component type\n";
			return;
		}
		rsize_t newIndex = mSize;
		mEntitytoIndexMap[entity] = newIndex;
		mIndextoEntityMap[newIndex] = entity;
		mComponentArray[newIndex] = component;
		++mSize;
	}

	void RemoveData(Entity entity)
	{

		if(mEntitytoIndexMap.find(entity) == mEntitytoIndexMap.end())
		{
			std::cout << "\nAttempting to delete an entity that does not exist.\n";
			return;
		}
		rsize_t IndexOfRemovedEntity = mEntitytoIndexMap[entity];
		rsize_t IndexOfLastElement = mSize - 1; 
		mComponentArray[IndexOfRemovedEntity] = mComponentArray[IndexOfLastElement];


		Entity LastEntity = mIndextoEntityMap[IndexOfLastElement];
		mEntitytoIndexMap[LastEntity] = IndexOfRemovedEntity;
		mIndextoEntityMap[IndexOfRemovedEntity] = LastEntity;

		mEntitytoIndexMap.erase(entity);
		mIndextoEntityMap.erase(IndexOfLastElement);
		--mSize;
	}

	T& GetData(Entity entity)
	{
		return mComponentArray[mEntitytoIndexMap[entity]];
	}

	void DestroyEntity(Entity entity) override
	{
		if (mEntitytoIndexMap.find(entity) != mEntitytoIndexMap.end()) 
		{
			RemoveData(entity);
		}
	}

private:
	std::array<T, MAX_ENTITIES> mComponentArray;
	std::unordered_map<Entity, size_t> mEntitytoIndexMap;
	std::unordered_map<size_t, Entity> mIndextoEntityMap;
	size_t mSize;
};

#endif //!COMPONENTARRAY_HPP