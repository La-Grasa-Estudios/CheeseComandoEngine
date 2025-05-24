#pragma once

#include <string>

namespace Javos
{
	const static std::string C_SONG_PATH_PREFIX = "fnf/songs/";
	const static std::string C_CHART_PATH_PREFIX = "fnf/data/";

	struct LoadChartParams
	{
		std::string ChartPath;
	};
}