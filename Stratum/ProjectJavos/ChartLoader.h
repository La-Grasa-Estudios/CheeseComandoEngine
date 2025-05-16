#pragma once

#include "ChartInfo.h"

namespace Javos
{
	class ChartLoader
	{
	public:
		static Chart LoadChart(const std::string& path);
	};
}