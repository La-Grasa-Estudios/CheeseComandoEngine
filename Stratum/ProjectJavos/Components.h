#pragma once

#include <cstdint>

namespace Javos
{

	static inline const char* C_NOTE_COMPONENT_NAME = "note_component";
	static inline const char* C_NOTE_HOLD_COMPONENT_NAME = "note_hold_component";

	struct NoteComponent
	{
		uint32_t Sustain = 0;
		uint32_t NoteType;
		float Time;
	};

	struct NoteHoldComponent
	{
		uint32_t NoteType;
		float Time;
		float HoldTime;
	};
}