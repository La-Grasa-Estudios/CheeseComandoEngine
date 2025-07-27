#pragma once

#include <Scene/Scene.h>

namespace Funkin
{
	enum class Easing
	{
		Linear,
		Random,
		SineIn,
		SineOut,
		SineInOut,
		ElasticIn,
		ElasticOut,
		ElasticInOut,
		BackIn,
		BackOut,
		BackInOut,
		Sine,
		SineAdd,
		Cosine,
	};

	struct ActionParameters
	{
		float duration;
		float amplitude = 1.0f;
		ActionParameters() = default;
		ActionParameters(const float dur);
		ActionParameters(const float dur, const float amp);
	};

	class TimedActionSystem : public Stratum::ISceneSystem
	{
	public:

		TimedActionSystem();
		~TimedActionSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing);

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void ClearActions();

	private:

		struct Action
		{
			Easing easing;
			uint8_t count;
			ActionParameters params;
			float time;
			float* floatPtr;
			float targetFloat[4];
			float srcFloat[4];
			std::function<void()> cb;
		};

		Stratum::Scene* mScene;

		std::vector<Action> mActions;

	};
}