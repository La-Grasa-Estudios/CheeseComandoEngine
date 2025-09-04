#pragma once

#include "znmsp.h"

BEGIN_ENGINE

namespace ECS
{
	typedef uint32_t edict_t;

	static inline constexpr uint32_t C_MAX_ENTITIES = 8192;
	static inline constexpr edict_t C_INVALID_ENTITY = 0;
}

END_ENGINE