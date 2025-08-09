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
};

extern GlobalVars* gpGlobals; // Stole the name straight from Source SDK lol

END_ENGINE