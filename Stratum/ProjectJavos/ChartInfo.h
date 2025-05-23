#pragma once

#include <string>
#include <vector>

namespace Javos
{
	struct ChartNote
	{
		float time;
		int noteType;
		float holdTime;
		bool mustHitSection = false;
	};
	struct ChartSection
	{
		std::vector<ChartNote> notes;
		int lengthInSteps = 16;
		int typeOfSection = 0;
		int bpm;
		bool changeBPM;
		bool mustHitSection = true;
		bool altAnim = false;
	};
	struct ChartInfo
	{
		bool needsVoices = false;
		uint32_t bpm = 0;
		float speed = 1.0f;
		std::string song;
		std::string stage;
	};
	struct ChartEvent
	{
		float EventTime;
		std::string EventName;
		std::string Arg1;
		std::string Arg2;
		bool Triggered = false;

		float castFloat(std::string token);
		int32_t castInteger(std::string token);
		bool castBoolean(std::string token);
	};
	struct Chart
	{
		ChartInfo info;
		std::vector<ChartEvent> events;
		std::vector<ChartSection> sections;
	};
}