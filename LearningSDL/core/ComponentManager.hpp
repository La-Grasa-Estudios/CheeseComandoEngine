#pragma once
#ifndef COMPONENTMANAGER_HPP
#define COMPONENTMANAGER_HPP
#include <unordered_map>
#include <memory>
#include "Components.hpp"
#include "ComponentArray.hpp"

class ComponentManager
{
public:
	template <typename T>
	void RegisterComponent()
	{
		const char* typeName = typeid(T).name();
		if(mComponentTypes.find(typeName) != mComponentTypes.end())
		{
			std::cout << "\nTriying to add an existent component to an entity\n";
			return;
		}

		mComponentTypes.insert({ typeName, mNextComponentType });
		mComponentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });
		++mNextComponentType;
	}

	template <typename T>
	ComponentType GetComponentType()
	{
		const char* typeName = typeid(T).name();

		if(mComponentArrays.find(typeName) == mComponentArrays.end())
		{
			std::cout << "\nTrying to get a nonexistent component\n";
			return -1;

		}
		return mComponentTypes[typeName];
	}
	template <typename T> 
	void AddComponent(Entity entity, T component)
	{
		GetComponentArray<T>()->InsertData(entity, component);
	}

	template <typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>()->RemoveData(entity);
	}

	template <typename T>
	T& GetComponent(Entity entity)
	{
		return GetComponentArray<T>()->GetData(entity);
	}

	void EntityDestroyed(Entity entity)
	{
		for (auto const& pair : mComponentArrays)
		{
			auto const& component = pair.second;
			component->DestroyEntity(entity);
		}
	}

private:
	std::unordered_map<const char*, ComponentType> mComponentTypes;
	std::unordered_map<const char*, std::shared_ptr<IComponentArray>> mComponentArrays;

	ComponentType mNextComponentType{};

	template <typename T>
	std::shared_ptr<ComponentArray<T>> GetComponentArray()
	{
		const char* typeName = typeid(T).name();

		if(mComponentTypes.find(typeName) == mComponentTypes.end())
		{
			std::cout << "\nAttempting to get a ComponentArray that doesn't exist\n";
			return nullptr;
		}
		return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeName]);
	}
};

#endif //!COMPONENTMANAGER_HPP