#pragma once

#ifdef _WIN32
#define EnApi __declspec(dllimport)
#endif
namespace Platform {

	enum class MessageBoxButtons {
		OK,
		OK_CANCEL
	};

	enum class MessageBoxIcons {
		Warning,
		Error
	};

	EnApi void ShowSystemMessageBox(const char* title, const char* text, MessageBoxButtons buttons, MessageBoxIcons icons);
	EnApi bool DoesCpuSupportSSE42();

}