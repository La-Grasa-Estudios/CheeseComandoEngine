#pragma once

#include "StageInfo.h"

#include <glm/ext.hpp>
#include <string>
#include <unordered_map>

namespace Stratum
{
	class Scene;
}

namespace Funkin
{
	class StageRegistry
	{
	public:

		static void Init(Stratum::Scene* scene);
		static void AddStage(const std::string& name);
		static void SetStage(const std::string& name);
		static LevelStage* GetCurrentStage();

	private:

		static void DisableStage(LevelStage* stage);
		static void EnableStage(LevelStage* stage);

		static inline Stratum::Scene* sScene;
		static inline LevelStage* sCurrentStage;
		static inline std::unordered_map<std::string, LevelStage> sStages;

	};
}