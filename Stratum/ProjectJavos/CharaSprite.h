#pragma once

#include <string>
#include <unordered_map>
#include <glm/ext.hpp>

namespace Stratum
{
	class Scene;
}

namespace Funkin
{

	class CharaSprite
	{

	public:

		CharaSprite(Stratum::Scene* scene, const std::string& file);
		void Update();
		void UpdateTransform();
		void PlayAnimation(const std::string& name);
		void SetEnabled(bool enabled);

		glm::vec2 CharaPosition = {};
		glm::vec2 CharaScale = glm::vec2(1.0f);

		int32_t CharaEntity;

	private:

		glm::vec2 mOriginalScale = {};

		bool mDoBeat = false;
		float mBeatAcumulator;
		uint32_t mFrameIndex = 0;

		std::unordered_map<std::string, glm::vec2> mOffsetMap;
		Stratum::Scene* mScene;
	};

}