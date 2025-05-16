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
	};

	struct Chart
	{
		ChartInfo info;
		std::vector<ChartSection> sections;
	};
}