#pragma once

#include <glm/ext.hpp>
#include <string>
#include <vector>

namespace Javos
{
	struct StagePropAnimation
	{

	};

	struct StageCharacter
	{
		int32_t zIndex = 0;
		glm::vec2 Position;
		glm::vec2 Scale;
		glm::vec2 CameraOffset;
	};

	struct StageProp
	{
		int32_t zIndex = 0;
		glm::vec2 Position;
		glm::vec2 Scale;
		glm::vec2 Scroll;
		float Opacity;
		float DanceEvery;
		std::string Name;
		std::string Asset;
	};

	struct LevelStage
	{
		std::string Name;
		float CameraZoom;
		StageCharacter Player;
		StageCharacter Gf;
		StageCharacter Oponent;
		std::vector<StageProp> Props;
	};

}