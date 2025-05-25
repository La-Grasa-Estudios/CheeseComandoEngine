#pragma once

#include <cstdint>

namespace Funkin
{
	class Conductor;
	class InGameSystem;

	struct GameState
	{
		uint32_t DoBeatEveryNthBeat = 4;
		uint32_t BeatOffset = 0;
		Conductor* pConductor;
		InGameSystem* pInGame;
		glm::vec2 CameraPosition;
		float CameraZoom = 1.0f;
		glm::vec2 PlayerPosition;
	};
}