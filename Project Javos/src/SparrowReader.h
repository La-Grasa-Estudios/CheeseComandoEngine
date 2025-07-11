#pragma once

#include "Entity/Components.h"

namespace Funkin
{
	class SparrowReader
	{
	public:
		static std::vector<Stratum::SpriteAnimator::AnimationFrame> readXML(std::string path, std::string nodeSearch, bool ignoreOffset, bool ignoreFrameSize = false);
	};
}