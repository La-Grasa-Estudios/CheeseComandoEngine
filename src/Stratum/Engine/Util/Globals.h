#pragma once

#include "znmsp.h"

BEGIN_ENGINE

// Source engine (definitely not source engine)
// Anyone?

struct GlobalVars
{
	uint64_t gametic;
	uint64_t tickRate;
	float deltaTime;
	float elapsedTime;
	size_t totalAllocated = 0;
	size_t totalAllocations = 0;
	size_t freedAllocations = 0;
};

extern GlobalVars* gpGlobals; // Stole the name straight from Source SDK lol

END_ENGINE