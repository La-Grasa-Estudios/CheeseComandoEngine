#pragma once

#include <cstdint>

namespace Javos
{
	class Conductor;
	struct GameState
	{
		uint32_t DoBeatEveryNthBeat = 4;
		uint32_t BeatOffset = 0;
		Conductor* pConductor;
		glm::vec2 CameraPosition;
		float CameraZoom = 1.0f;
		glm::vec2 PlayerPosition;
	};
}