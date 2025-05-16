#pragma once

#include "ChartInfo.h"

namespace Stratum
{
	class Scene;
}

namespace Javos
{
	class Conductor
	{
	public:
		float SongTime;

		Chart chart;

		void LoadChart(Stratum::Scene* scene, const std::string& path);
		void Update(Stratum::Scene* scene);

	private:

		void SpawnNote(Stratum::Scene* scene, ChartNote note);
		bool CheckNoteInput(float time);

	};
}