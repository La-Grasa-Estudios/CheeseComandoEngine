#pragma once

#include <cstdint>

namespace Funkin
{

	static inline const char* C_NOTE_COMPONENT_NAME = "note_component";
	static inline const char* C_NOTE_HOLD_COMPONENT_NAME = "note_hold_component";
	static inline const char* C_ANIMATED_EFFECT_COMPONENT_NAME = "animated_effect_component";
	static inline const char* C_STAGE_PROP_COMPONENT_NAME = "stage_prop";

	struct NoteComponent
	{
		uint32_t Sustain = 0;
		uint32_t NoteType;
		uint32_t NoteIndex = 0;
		uint32_t SectionIndex = 0;
		float Time;
		bool IsOponent;
	};

	struct NoteHoldComponent
	{
		uint32_t SustainEndSprite;
		uint32_t NoteType;
		float Time;
		float HoldTime;
	};

	struct AnimatedEffectComponent /// Just here because i need something to iterate with
	{
		uint8_t dummy;
	};

	struct StagePropComponent
	{
		glm::vec2 Scroll;
		glm::vec2 Position;
		float DanceEvery;
		std::string StageName;
	};
}