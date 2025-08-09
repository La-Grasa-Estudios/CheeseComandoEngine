#pragma once

#include "znmsp.h"

#include <vector>
#include <string>
#include <functional>

#include "Logger.h"

BEGIN_ENGINE

class Console : LogReceiver {

public:

	Console();

	/// <summary>
	/// Returns true when the console wants control of the keyboard inputs
	/// </summary>
	/// <returns></returns>
	bool Draw();
	void Log(std::string_view msg, LogLevel level) override;
	void SetLogCallback(std::function<void(const std::string&)> callback);
	bool Focused();

private:

	std::vector<std::string> m_ConsoleLog;

	char m_ConBuffer[512];
	bool m_HasConsoleLogCallBack;
	bool m_IsFocused;
	std::function<void(const std::string&)> m_ConsoleLogCallBack;

};

END_ENGINE