#pragma once

#include "CharaSprite.h"

#include <string>

namespace Stratum
{
	class Scene;
}

namespace Funkin
{
	class CharaRegistry
	{
	public:
		static void Init(Stratum::Scene* scene);
		static void AddCharacter(const std::string& name);
		static void SetPlayerCharacter(const std::string& name);
	private:
		static Stratum::Scene* sScene;
	};
}