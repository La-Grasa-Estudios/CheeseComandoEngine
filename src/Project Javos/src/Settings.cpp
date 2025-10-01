#include "Settings.h"
using namespace Funkin;

#include <fstream>
#include <json/json.hpp>

Funkin::Settings::Settings()
{

}

void Funkin::Settings::Init()
{
	if (!s_Settings)
		s_Settings = new Settings();
}

void Funkin::Settings::LoadFromFile(const std::string& filename)
{
	std::ifstream in(filename);
	if (!in.is_open())
		return;
	nlohmann::json json;
	in >> json;
	in.close();
	for (auto& [key, value] : json.items())
	{
		settings[key] = value.get<int>();
	}
}

void Funkin::Settings::SaveToFile(const std::string& filename) const
{
	nlohmann::json json;
	for (auto& kv : settings)
	{
		json[kv.first] = kv.second.intValue;
	}
	std::ofstream out(filename);
	out << json.dump(4);
	out.close();
}

SettingsValue& Funkin::Settings::Get(const char* key, SettingsValue defaultValue)
{
	if (settings.find(key) != settings.end())
		return settings.at(key);
	settings[key] = defaultValue;
	return defaultValue;
}
