#pragma once

#include <cstdint>

namespace Javos
{

	static inline const char* C_NOTE_COMPONENT_NAME = "note_component";

	struct NoteComponent
	{
		uint32_t NoteType;
		float Time;
	};
}