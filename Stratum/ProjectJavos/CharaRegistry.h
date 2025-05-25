#pragma once

#include "CharaSprite.h"

#include <Core/Ref.h>

#include <string>
#include <unordered_map>

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
		static void Update();
		static CharaSprite* GetCharacter(const std::string& name);
	private:
		static inline Stratum::Scene* sScene;
		static inline std::unordered_map<std::string, Stratum::Ref<CharaSprite>> sCharaSprites;
	};
}