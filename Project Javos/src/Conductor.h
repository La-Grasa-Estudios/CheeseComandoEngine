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

	class Conductor : public Stratum::ISceneSystem
	{
	public:

		float SongTime;
		float BeatCountF;
		uint32_t BeatCount;
		bool IsPaused = false;

		Chart chart;

		Conductor();

		void Init(Stratum::Scene* scene) override;
		void PostUpdate(Stratum::Scene* scene) override;

		void LoadChart(Stratum::Scene* scene, const std::string& path);
		void Update(Stratum::Scene* scene);

		void RegisterEventHandler(const std::string& eventName, ChartEventHandler handler);
		void AddScriptedEvent(int step, ScriptedEvent event);

		float StepsToSeconds(uint32_t stepCount);
		float GetConductorBeatMultiplier();
		ChartNote& GetNoteByIndex(uint32_t sectionIndex, uint32_t noteIndex);
		std::string GetPlayer1Name() const;
		std::string GetPlayer2Name() const;

		uint32_t GetStepCount();

		bool EnableBot = false;
		bool BotPlay = false;
		int32_t PlayerScore = 0;

	private:

		void OnStep();

		struct ScriptedEventContainer
		{
			ScriptedEvent event;
			bool executed;
			int stepCount;
		};

		void SpawnNote(Stratum::Scene* scene, ChartNote note, uint32_t index, uint32_t sectionIndex);
		void SpawnNoteSplash(Stratum::Scene* scene, uint32_t noteType);
		void SpawnSustainCover(Stratum::Scene* scene, uint32_t noteType);

		void AddScoreNoteHit(int32_t time);

		uint32_t mStepCount = 0;
		float mStepAccumulator = 0.0f;

		Stratum::ECS::edict_t mScoreTextEntity;
		Stratum::ECS::edict_t mSubtitlesTextEntity;

		std::array<Stratum::ECS::edict_t, 4> mSustainHeld;
		std::array<Stratum::ECS::edict_t, 4> mNoteCovers;

		std::array<Stratum::SpriteAnimator::Animation, 8> mNoteSplashesAnimations;
		std::array<Stratum::SpriteAnimator::Animation, 4> mNoteCoverAnimations;
		std::array<Stratum::SpriteAnimator::Animation, 4> mNoteCoverEndAnimations;
		
		std::unordered_map<std::string, ChartEventHandler> mEventHandlers;
		std::vector<ScriptedEventContainer> mScriptedEvents;

	};
}