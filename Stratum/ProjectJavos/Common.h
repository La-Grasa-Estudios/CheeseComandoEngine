#pragma once

#include <string>

namespace Javos
{
	const static std::string C_SONG_PATH_PREFIX = "fnf/songs/";
	const static std::string C_CHART_PATH_PREFIX = "fnf/data/";
	const static std::string C_STAGE_PATH_PREFIX = "fnf/stages/";

	std::string GenerateAssetPath(const std::string& prefix, const std::string& file, const std::string& extension);

	struct LoadChartParams
	{
		std::string ChartPath;
	};
}