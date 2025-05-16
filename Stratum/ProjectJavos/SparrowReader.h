#pragma once

#include "Entity/Components.h";

namespace Javos
{
	class SparrowReader
	{
	public:
		static std::vector<Stratum::SpriteAnimator::AnimationFrame> readXML(std::string path, std::string nodeSearch, bool ignoreOffset);
	};
}