#pragma once
#ifndef SYSTEMMANAGER_HPP
#define SYSTEMMANAGER_HPP

#include <memory>
#include <unordered_map>
#include "System.hpp"
#include "Components.hpp"

class SystemManager : public System
{
public:
	template <typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		const char* typeName = typeid(T).name();

		if (mSystems.find(typeName) != mSystems.end())
		{
			std::cout << "\nTrying to add an existing system\n";
			return nullptr;
		}

		auto system = std::make_shared<T>();
		mSystems.insert({ typeName, system });
		return system;
	}

	template <typename T>
	void SetSignature(Signature signature)
	{
		const char* typeName = typeid(T).name();

		if (mSystems.find(typeName) == mSystems.end())
		{
			std::cout << "\nTrying to add a signature before register the system\n";
			return;
		}

		mSignatures.insert({ typeName, signature });

	}

	void EntityDestroyed(Entity entity)
	{
		for (auto const& pair : mSystems)
		{
			auto const& system = pair.second;
			system->mEntities.erase(entity);
		}
	}

	void EntitySignatureChanged(Entity entity, Signature enttSignature)
	{
		for (auto const& pair : mSystems)
		{
			auto const& type = pair.first;
			auto const& system = pair.second;
			auto const& SystemSignature = mSignatures[type];

			if ((SystemSignature & enttSignature) == SystemSignature)
			{
				system->mEntities.insert(entity);
			}
			else
			{
				system->mEntities.erase(entity);
			}
		}
	}

private:
	std::unordered_map<const char*, Signature> mSignatures{};
	std::unordered_map<const char*, std::shared_ptr<System>> mSystems{};
};

#endif // !SYSTEMMANAGER_HPP
