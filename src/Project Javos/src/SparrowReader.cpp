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

std::vector<Stratum::SpriteAnimator::AnimationFrame> Funkin::SparrowReader::readXML(std::string path, std::string nodeSearch, bool ignoreOffset, bool ignoreFrameSize)
{
	ignoreOffset = false;

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
			size_t index_x = s.find("x=");
			size_t index_y = s.find("y=");
			size_t index_width = s.find("width=");
			size_t index_height = s.find("height=");
			size_t index_frame_width = s.find("frameWidth=");
			size_t index_frame_height = s.find("frameHeight=");
			size_t index_offset_x = s.find("frameX=");
			size_t index_offset_y = s.find("frameY=");
			size_t index_rotated = s.find("rotated=");

			std::string xPos = deleteSeparator(s.substr(index_x+2, 5), '"');
			std::string yPos = deleteSeparator(s.substr(index_y+2, 5), '"');
			std::string wPos = deleteSeparator(s.substr(index_width+6, 11-6), '"');
			std::string hPos = deleteSeparator(s.substr(index_height+7, 12-7), '"');

			Stratum::SpriteAnimator::AnimationFrame frame = {
				{ glm::ivec2(std::atoi(xPos.c_str()), std::atoi(yPos.c_str())) , glm::ivec2(std::atoi(wPos.c_str()), std::atoi(hPos.c_str())) },
				glm::ivec2(0, 0),
			};

			frame.FrameSize = frame.Rect.size;

			if (ignoreOffset || ignoreFrameSize)
			{
				frame.Offset = { 0, 0 };
			}
			else 
			{
				if (index_offset_x != std::string::npos)
				{
					std::string xOff = deleteSeparator(s.substr(index_offset_x + 7, 12 - 7), '"');
					frame.Offset.x = std::atoi(xOff.c_str());
				}
				if (index_offset_y != std::string::npos)
				{
					std::string yOff = deleteSeparator(s.substr(index_offset_y + 7, 12 - 7), '"');
					frame.Offset.y = std::atoi(yOff.c_str());
				}
			}

			if (!ignoreFrameSize)
			{
				if (index_frame_width != std::string::npos)
				{
					std::string width = deleteSeparator(s.substr(index_frame_width + 11, 5), '"');
					frame.FrameSize.x = std::atoi(width.c_str());
				}

				if (index_frame_height != std::string::npos)
				{
					std::string height = deleteSeparator(s.substr(index_frame_height + 12, 5), '"');
					frame.FrameSize.y = std::atoi(height.c_str());
				}
			}

			if (index_rotated != std::string::npos)
			{
				std::string rotated = deleteSeparator(s.substr(index_rotated + 8, 6), '"');
				if (rotated.compare("true") == 0)
				{
					frame.Rotated = true;
				}
			}

			if (frame.Rect.size.x == 0 || frame.Rect.size.y == 0)
				continue;

			frames.push_back(frame);
		}
	}

	return frames;
	
}
