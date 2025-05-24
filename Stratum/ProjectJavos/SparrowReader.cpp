#include "SparrowReader.h"

#include "VFS/ZVFS.h"

std::string deleteSeparator(std::string str, char separator)
{
	std::string str1;
	for (auto c : str)
	{
		if (c == separator)
			continue;
		str1 += c;
	}
	return str1;
}

std::vector<Stratum::SpriteAnimator::AnimationFrame> Javos::SparrowReader::readXML(std::string path, std::string nodeSearch, bool ignoreOffset)
{

	std::vector<Stratum::SpriteAnimator::AnimationFrame> frames;
	std::vector<std::string> data;

	auto stream = Stratum::ZVFS::GetFile(path.c_str());

	std::string line;
	while (std::getline(*stream->Stream(), line))
	{
		data.push_back(line);
	}

	for (auto s : data)
	{
		if (s.find(nodeSearch.c_str()) != std::string::npos)
		{
			int index_x = s.find("x=");
			int index_y = s.find("y=");
			int index_width = s.find("width=");
			int index_height = s.find("height=");
			size_t index_offset_x = s.find("frameX=");
			size_t index_rotated = s.find("rotated=");

			std::string xPos = deleteSeparator(s.substr(index_x+2, 5), '"');
			std::string yPos = deleteSeparator(s.substr(index_y+2, 5), '"');
			std::string wPos = deleteSeparator(s.substr(index_width+6, 11-6), '"');
			std::string hPos = deleteSeparator(s.substr(index_height+7, 12-7), '"');

			Stratum::SpriteAnimator::AnimationFrame frame = {
				{ glm::ivec2(std::atoi(xPos.c_str()), std::atoi(yPos.c_str())) , glm::ivec2(std::atoi(wPos.c_str()), std::atoi(hPos.c_str())) },
				glm::ivec2(0, 0),
			};

			if (ignoreOffset)
			{
				frame.Offset = { 0, 0 };
			}
			else if (index_offset_x != std::string::npos)
			{
				std::string xOff = deleteSeparator(s.substr(index_offset_x + 7, 12 - 7), '"');
				frame.Offset = glm::ivec2(std::atoi(xOff.c_str()), 0);
			}
			if (index_rotated != std::string::npos)
			{
				std::string rotated = deleteSeparator(s.substr(index_rotated + 8, 6), '"');
				if (rotated.compare("true") == 0)
				{
					frame.Rotated = true;
				}
			}

			frames.push_back(frame);
		}
	}

	return frames;
	
}
