#pragma once

#include "znmsp.h"

BEGIN_ENGINE

namespace Utils
{
	class CpuUtil
	{
	public:
		static inline bool SupportsSSE41 = false;
		static inline bool SupportsAVX2 = false;
		static inline bool SupportsAVX512F = false;
		static inline bool SupportsAVX512BW = false;
		CpuUtil();
	};
}

END_ENGINE