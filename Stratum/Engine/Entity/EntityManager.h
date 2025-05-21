#pragma once

#include "Common.h"

#include <functional>
#include <array>

BEGIN_ENGINE

namespace ECS
{
	typedef std::function<void(edict_t)> EntityRemovalEvent;

	class EntityManager
	{
	public:

		EntityManager();

		edict_t CreateEntity();
		void DestroyEntity(edict_t entity);

		void DestroyAll();

		bool IsValid(edict_t entity);

		void RegisterRemoval(const EntityRemovalEvent& func);
			
		uint32_t LiveEntities = 0;
		uint32_t MaxEntities = 0;

	private:

		std::vector<EntityRemovalEvent> mRemovals;
		std::array<bool, C_MAX_ENTITIES> mValidEntities;
		uint32_t mSearchStart = 0;
	};
}

END_ENGINE