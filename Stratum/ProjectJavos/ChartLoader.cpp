#include "ChartLoader.h"

#include "Core/Logger.h"

#include "VFS/ZVFS.h"

#include <json/json.hpp>

Javos::Chart Javos::ChartLoader::LoadChart(const std::string& path)
{

	Stratum::RefBinaryStream stream = Stratum::ZVFS::GetFile(path.c_str());

	std::string str = stream->Str();

	Chart chart{};

	try
	{
		nlohmann::json js = nlohmann::json::parse(str);

		auto song = js["song"];

		chart.info.bpm = song["bpm"];
		chart.info.needsVoices = song["needsVoices"];
		chart.info.song = song["song"];
		chart.info.speed = song["speed"];

		auto sections = song["notes"];

		for (auto section : sections)
		{
			ChartSection sec;
			sec.mustHitSection = section["mustHitSection"];

			auto sectionNotes = section["sectionNotes"];

			for (auto notes : sectionNotes)
			{

				ChartNote note;
				note.time = notes[0] / 1000.0f;
				note.noteType = notes[1];
				note.holdTime = notes[2] / 1000.0f;
				note.mustHitSection = sec.mustHitSection;
				sec.notes.push_back(note);

			}

			chart.sections.push_back(sec);
		}

	}
	catch (const std::exception& e)
	{
		Z_ERROR(e.what());
	}

    return chart;
}
