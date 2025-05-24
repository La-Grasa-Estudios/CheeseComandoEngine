#pragma once

#include "znmsp.h"
#include "EntityManager.h"

#include <array>
#include <assert.h>

BEGIN_ENGINE

namespace ECS
{

	class ComponentManager_Interface
	{
	public:
		virtual void Init(EntityManager* pManager) = 0;
	};

	template<typename Component>
	class ComponentManager : public ComponentManager_Interface
	{
	public:

		ComponentManager() = default;

		void Init(EntityManager* pManager)
		{
			memset(mAllocatedArray.data(), 0, sizeof(mAllocatedArray));
			memset(mComponentArray.data(), 0, sizeof(Component) * C_MAX_ENTITIES);
			mEntities.reserve(C_MAX_ENTITIES);

			pManager->RegisterRemoval([this](edict_t edict)
				{
					this->Remove(edict);
				});

			for (int i = 0; i < C_MAX_ENTITIES; i++)
			{
				new (&mComponentArray[i]) Component();
			}
		}

		Component& Create(edict_t edict)
		{
			mAllocatedArray[edict - 1] = true;

			mLookup[edict] = mEntities.size();
			mEntities.push_back(edict);

			new (&mComponentArray[edict - 1]) Component();

			return mComponentArray[edict - 1];
		}

		Component& Get(edict_t edict)
		{
			assert(HasComponent(edict));
			return mComponentArray[edict - 1];
		}

		void Remove(edict_t edict)
		{
			if (!edict || !HasComponent(edict)) return;

			auto el = mLookup.find(edict);

			if (el != mLookup.end())
			{

				const size_t index = el->second;

				mEntities.erase(mEntities.begin() + index);

				for (auto& kp : mLookup)
				{
					if (kp.second > index)
					{
						kp.second -= 1;
					}
				}

				mLookup.erase(edict);

				mComponentArray[edict - 1] = {};
				mAllocatedArray[edict - 1] = false;
			}
		}

		bool HasComponent(edict_t edict)
		{
			if (!edict) return false;
			return mAllocatedArray[edict - 1];
		}

		std::vector<edict_t>& GetEntities()
		{
			return mEntities;
		}

	private:

		std::vector<edict_t> mEntities;
		std::unordered_map<edict_t, size_t> mLookup;

		std::array<bool, C_MAX_ENTITIES> mAllocatedArray;
		std::array<Component, C_MAX_ENTITIES> mComponentArray;
	};
}

END_ENGINE