#pragma once

#include "GlobalVar.h"
#include "Ref.h"

#include <unordered_map>

BEGIN_ENGINE

class VarRegistry {

public:

	static ConsoleVar* RegisterConsoleVar(std::string module, std::string name, VarType type);
	static ConsoleVar* GetConsoleVar(std::string module, std::string name);
	static void RunCfg(std::string file);
	static void Cleanup();
	static bool ParseConsoleVar(std::string in, std::string& log);
	static std::vector<std::string> GetConVars(int maxConVars, std::string& filter);

private:

	static void InitCVar(ConsoleVar& var);
	inline static std::unordered_map<std::string, Ref<ConsoleVar>> s_GlobalVars;

};

END_ENGINE