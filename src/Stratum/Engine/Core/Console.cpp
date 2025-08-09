#include "Console.h"

#include "Thirdparty/imgui/imgui.h"
#include "Thirdparty/imgui/imgui_internal.h"

#include "VarRegistry.h"
#include "Input/Input.h"

using namespace ENGINE_NAMESPACE;

Console::Console()
{
	memset(m_ConBuffer, 0, sizeof(m_ConBuffer));
	Logger::SetLogger(this);
	m_HasConsoleLogCallBack = false;
	m_IsFocused = false;
}

bool Console::Draw()
{

	bool captureKeyboard = false;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("Console");

	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 newPos = { 0.0f, winSize.y - 20 };
	ImGui::SetCursorPos(newPos);

	ImGui::SetNextItemWidth(winSize.x);

	if (ImGui::InputText("ConVars", m_ConBuffer, 512, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AlwaysOverwrite)) {
		std::string var = m_ConBuffer;
		std::string log;
		VarRegistry::ParseConsoleVar(var, log);
		if (!log.empty()) {
			m_ConsoleLog.push_back(log);
		}
		else {
			m_ConsoleLog.push_back(var);
		}
		memset(m_ConBuffer, 0, sizeof(m_ConBuffer));
	}

	newPos.y -= 2.0f;

	static int cursor = -1;

	std::string text = m_ConBuffer;
	std::vector<std::string> vars = VarRegistry::GetConVars(10, text);

	ImGuiStyle& style = ImGui::GetStyle();
	

	if (ImGui::IsItemActive()) {
		
		captureKeyboard = true;

		if (Input::GetKeyDown(KeyCode::UP)) {
			cursor--;
		}
		if (Input::GetKeyDown(KeyCode::DOWN)) {
			cursor++;
		}
		if (cursor < 0) {
			cursor = vars.size() - 1;
		}
		if (cursor > vars.size() - 1) {
			cursor = 0;
		}
		if (Input::GetKeyDown(KeyCode::TAB) && !vars.empty()) {
			std::string filter = vars[cursor];
			size_t pos = filter.find_first_of(" ");
			size_t seekPos = filter.size();
			if (pos != std::string::npos) {
				seekPos = pos;
			}
			memset(m_ConBuffer, 0, sizeof(m_ConBuffer));
			memcpy(m_ConBuffer, filter.data(), seekPos);
			ImGui::ClearActiveID();
		}
	}

	for (int i = m_ConsoleLog.size() - 1; i >= glm::max(0, (int)m_ConsoleLog.size() - 30); i--) {
		ImVec2 size = ImGui::CalcTextSize(m_ConsoleLog[i].c_str());
		newPos.y -= size.y;
		ImGui::SetCursorPos(newPos);
		ImGui::Text(m_ConsoleLog[i].c_str());
	}

	newPos = { 0.0f, winSize.y - 20 };
	newPos.y -= 2.0f;

	for (int i = vars.size() - 1; i >= 0; i--) {
		ImVec2 size = ImGui::CalcTextSize(vars[i].c_str());
		ImVec2 pos = ImGui::GetWindowPos();
		newPos.y -= size.y;

		uint32_t col = 0;

		if (cursor == i) {
			newPos.x = 4.0f;
			col = 1;
		}
		else {
			newPos.x = 0.0f;
		}

		uint32_t color = ImGui::GetColorU32(style.Colors[ImGuiCol_WindowBg]) << col;

		ImVec2 pMin = { pos.x, newPos.y + pos.y };
		ImVec2 pMax = { winSize.x + pMin.x, size.y + pMin.y };

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(
			pMin,
			pMax,
			color);

		ImGui::SetCursorPos(newPos);
		ImGui::Text(vars[i].c_str());
	}

	ImGui::End();
	ImGui::PopStyleVar();
	m_IsFocused = captureKeyboard;
	return captureKeyboard;
}

void Console::Log(std::string_view msg, LogLevel level)
{
	Lock();
	std::string fmt = std::format("{}{}", Format(level), msg.data());
	if (m_HasConsoleLogCallBack) {
		m_ConsoleLogCallBack(fmt);
	}
	m_ConsoleLog.push_back(fmt);
	Release();
	LogReceiver::Log(msg, level);
}

void Console::SetLogCallback(std::function<void(const std::string&)> callback)
{
	m_HasConsoleLogCallBack = true;
	m_ConsoleLogCallBack = callback;
}

bool Console::Focused()
{
	return m_IsFocused;
}
