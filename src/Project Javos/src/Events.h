#pragma once
#include <cstdint>

namespace Funkin
{
	struct NoteEvent
	{
		bool IsOponent;
		bool IsMiss;
		bool IsSustain;
		uint32_t NoteType;
		uint32_t NoteIndex;
		uint32_t SectionIndex;
	};
}