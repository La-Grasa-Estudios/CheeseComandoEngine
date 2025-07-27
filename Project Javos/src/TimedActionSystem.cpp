#include "TimedActionSystem.h"

#include <Util/Globals.h>

const float PI = glm::pi<float>();

static inline float easeInElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0.0f
		? 0
		: x == 1.0f
		? 1
		: -glm::pow(2, 10 * x - 10) * glm::sin((x * 10 - 10.75) * c4);
}

static inline float easeOutElastic(float x) {
	float c4 = (2 * PI) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: glm::pow(2, -10 * x) * glm::sin((x * 10 - 0.75) * c4) + 1;
}

static inline float easeInOutElastic(float x) {
	float c5 = (2 * PI) / 4.5;

	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5
		? -(glm::pow(2, 20 * x - 10) * glm::sin((20 * x - 11.125) * c5)) / 2
		: (glm::pow(2, -20 * x + 10) * glm::sin((20 * x - 11.125) * c5)) / 2 + 1;
}

static inline float easeInBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return c3 * x * x * x - c1 * x * x;
}

static inline float easeOutBack(float x) {
	float c1 = 1.70158;
	float c3 = c1 + 1;

	return 1 + c3 * glm::pow(x - 1, 3) + c1 * glm::pow(x - 1, 2);
}

static inline float easeInOutBack(float x) {
	float c1 = 1.70158;
	float c2 = c1 * 1.525;

	return x < 0.5
		? (glm::pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
		: (glm::pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
}

Funkin::TimedActionSystem::TimedActionSystem()
{

}

Funkin::TimedActionSystem::~TimedActionSystem()
{
}

void Funkin::TimedActionSystem::Init(Stratum::Scene* scene)
{

}

void Funkin::TimedActionSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Funkin::TimedActionSystem::RenderImGui(Stratum::Scene* scene)
{

}

void Funkin::TimedActionSystem::Update(Stratum::Scene* scene)
{
	for (int i = 0; i < mActions.size(); i++)
	{
		Action& act = mActions[i];

		if (act.time == 0.0f)
		{
			for (int i = 0; i < act.count; i++)
			{
				act.srcFloat[i] = act.floatPtr[i];
			}
		}

		act.time += Stratum::gpGlobals->deltaTime;

		float interp = glm::clamp(act.time / act.params.duration, 0.0f, 1.0f);

		for (int i = 0; i < act.count; i++)
		{
			float val = 0.0f;

			switch (act.easing)
			{
			case Easing::Linear:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], interp);
				break;
			case Easing::Random:
			{
				float randomVal = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::clamp(interp + randomVal, 0.0f, 1.0f));
			}
			break;
			case Easing::SineIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], 1 - glm::cos((interp * PI) / 2));
				break;
			case Easing::SineOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::sin((interp * PI) / 2.0f));
				break;
			case Easing::SineInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], -(glm::cos(PI * interp) - 1) / 2.0f);
				break;
			case Easing::ElasticIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInElastic(interp));
				break;
			case Easing::ElasticOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeOutElastic(interp));
				break;
			case Easing::ElasticInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInOutElastic(interp));
				break;
			case Easing::BackIn:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInBack(interp));
				break;
			case Easing::BackOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeOutBack(interp));
				break;
			case Easing::BackInOut:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], easeInOutBack(interp));
				break;
			case Easing::Sine:
				interp = glm::sin((interp * PI) / 2.0f);
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::sin(interp * (PI * 2) * act.params.amplitude));
				break;
			case Easing::SineAdd:
				val = act.srcFloat[i] + glm::sin(interp * PI * act.params.amplitude) * act.targetFloat[i];
				break;
			case Easing::Cosine:
				val = glm::mix(act.srcFloat[i], act.targetFloat[i], glm::cos(interp * (PI * 2) * act.params.amplitude));
				break;
			default:
				break;
			}

			act.floatPtr[i] = val;
		}

		if (act.time >= act.params.duration)
		{
			if (act.cb)
			{
				act.cb();
			}
			mActions.erase(mActions.begin() + i);
			i--;
			continue;
		}
	}
}

// Some hot shit incoming, but it does the job

void Funkin::TimedActionSystem::PushAction(float* dst, float targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 1;
	action.params = params;
	action.easing = easing;
	action.floatPtr = dst;
	action.targetFloat[0] = targetVal;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 2;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 3;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing)
{
	Action action{};
	action.count = 4;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.targetFloat[3] = targetVal.w;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(float* dst, float targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 1;
	action.params = params;
	action.easing = easing;
	action.floatPtr = dst;
	action.targetFloat[0] = targetVal;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 2;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 3;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing, std::function<void()> cb)
{
	Action action{};
	action.count = 4;
	action.params = params;
	action.easing = easing;
	action.floatPtr = glm::value_ptr(*dst);
	action.targetFloat[0] = targetVal.x;
	action.targetFloat[1] = targetVal.y;
	action.targetFloat[2] = targetVal.z;
	action.targetFloat[3] = targetVal.w;
	action.cb = cb;
	mActions.push_back(action);
}

void Funkin::TimedActionSystem::ClearActions()
{
	mActions.clear();
}

// End of hot shit

Funkin::ActionParameters::ActionParameters(const float dur) : duration(dur)
{
}

Funkin::ActionParameters::ActionParameters(const float dur, const float amp) : duration(dur), amplitude(amp)
{
}