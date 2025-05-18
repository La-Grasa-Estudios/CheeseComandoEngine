#pragma once

#include "ChartInfo.h"

#include <Scene/Scene.h>
#include <array>

namespace Stratum
{
	class Scene;
}

namespace Javos
{
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

	private:

		void SpawnNote(Stratum::Scene* scene, ChartNote note);
		bool CheckNoteInput(float time);

		uint64_t mHitNoteEvent;
		std::array<Stratum::ECS::edict_t, 4> mSustainHeld;

	};
}