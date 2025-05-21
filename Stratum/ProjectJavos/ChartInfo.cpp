#include "ChartInfo.h"

#include <regex>

float Javos::ChartEvent::castFloat(std::string token)
{
	bool isFloat = std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));

	if (isFloat)
	{
		return std::atof(token.c_str());
	}

	return 0.0f;
}

int32_t Javos::ChartEvent::castInteger(std::string token)
{
	bool isInteger = token.find_first_not_of("-1234567890") == std::string::npos;

	if (isInteger)
	{
		return std::atoi(token.c_str());
	}

	return 0;
}

bool Javos::ChartEvent::castBoolean(std::string token)
{
	if (castInteger(token))
	{
		return true;
	}

	if (token.compare("true") == 0)
	{
		return true;
	}

	return false;
}
