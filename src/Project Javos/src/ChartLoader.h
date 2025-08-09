#pragma once

#include "ChartInfo.h"

namespace Funkin
{
	class ChartLoader
	{
	public:
		static Chart LoadChart(const std::string& path);
	};
}