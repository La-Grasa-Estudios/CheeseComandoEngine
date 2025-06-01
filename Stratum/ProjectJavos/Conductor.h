#pragma once

#include "ChartInfo.h"

#include <Scene/Scene.h>
#include <array>
#include <vector>

namespace Stratum
{
	class Scene;
}

namespace Funkin
{

	typedef std::function<void(ChartEvent&)> ChartEventHandler;
	typedef std::function<void()> ScriptedEvent;

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

	class Conductor : public Stratum::ISceneSystem
	{
	public:

		float SongTime;
		float BeatCountF;
		uint32_t BeatCount;

		Chart chart;

		Conductor();

		void Init(Stratum::Scene* scene) override;
		void PostUpdate(Stratum::Scene* scene) override;

		void LoadChart(Stratum::Scene* scene, const std::string& path);
		void Update(Stratum::Scene* scene);

		void RegisterEventHandler(const std::string& eventName, ChartEventHandler handler);
		void AddScriptedEvent(int step, ScriptedEvent event);

		float GetConductorBeatMultiplier();
		ChartNote& GetNoteByIndex(uint32_t sectionIndex, uint32_t noteIndex);
		std::string GetPlayer1Name() const;
		std::string GetPlayer2Name() const;

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing);

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);

		uint32_t GetStepCount();

		bool EnableBot = false;
		int32_t PlayerScore = 0;

	private:

		void OnStep();

		struct ScriptedEventContainer
		{
			ScriptedEvent event;
			bool executed;
			int stepCount;
		};

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

		void SpawnNote(Stratum::Scene* scene, ChartNote note, uint32_t index, uint32_t sectionIndex);
		void SpawnNoteSplash(Stratum::Scene* scene, uint32_t noteType);
		void SpawnSustainCover(Stratum::Scene* scene, uint32_t noteType);

		void AddScoreNoteHit(uint32_t time);

		uint32_t mStepCount = 0;
		float mStepAccumulator = 0.0f;

		uint64_t mHitNoteEvent;
		uint64_t mSustainNoteEvent;
		uint64_t mMissNoteEvent;
		uint64_t mOponentNoteEvent;

		std::array<Stratum::ECS::edict_t, 4> mSustainHeld;
		std::array<Stratum::ECS::edict_t, 4> mNoteCovers;

		std::array<Stratum::SpriteAnimator::Animation, 8> mNoteSplashesAnimations;
		std::array<Stratum::SpriteAnimator::Animation, 4> mNoteCoverAnimations;
		std::array<Stratum::SpriteAnimator::Animation, 4> mNoteCoverEndAnimations;
		
		std::unordered_map<std::string, ChartEventHandler> mEventHandlers;
		std::vector<ScriptedEventContainer> mScriptedEvents;
		std::vector<Action> mActions;

	};
}