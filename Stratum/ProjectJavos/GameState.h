#pragma once

#include <cstdint>

namespace Javos
{
	struct GameState
	{
		uint32_t DoBeatEveryNthBeat = 4;
		uint32_t BeatOffset = 0;
	};
}