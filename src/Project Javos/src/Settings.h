#pragma once

#include <string>
#include <unordered_map>

namespace Funkin
{
	struct SettingsValue
	{
		union
		{
			int intValue;
			float floatValue;
			bool boolValue;
		};
		SettingsValue() = default;
		SettingsValue(int v) : intValue(v) {}
		SettingsValue(float v) : floatValue(v) {}
		SettingsValue(bool v) : boolValue(v) {}
		operator int() const { return intValue; }
		operator float() const { return floatValue; }
		operator bool() const { return boolValue; }
	};
	struct Settings
	{
		Settings();
		static void Init();
		void LoadFromFile(const std::string& filename);
		void SaveToFile(const std::string& filename) const;
		SettingsValue& Get(const char* key, SettingsValue defaultValue);
		std::unordered_map<std::string, SettingsValue> settings;
		inline static Settings* s_Settings;
	};
}