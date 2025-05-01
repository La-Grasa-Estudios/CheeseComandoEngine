#pragma once

#include "znmsp.h"

#include <functional>

BEGIN_ENGINE

enum class VarType {
	Int,
	Float,
	Bool,
	String,
	Void,
};

class ConsoleVar {

public:

	VarType type;
	char* data = 0;

	inline int32_t asInt();
	inline float_t asFloat();
	inline bool asBool();
	inline std::string str();

	void setOnModifyCallback(std::function<void(ConsoleVar&)> callback);

	ConsoleVar* set(int32_t i);
	ConsoleVar* set(float_t i);
	ConsoleVar* set(bool i);
	ConsoleVar* set(std::string_view i);
	ConsoleVar* set(const char* i);

	operator bool();
	operator int32_t();
	operator float_t();
	operator std::string();

	std::function<void(ConsoleVar&, std::string& args)> func;

private:

	bool hasCallBack = false;
	std::function<void(ConsoleVar&)> callback;
	size_t dataSize = 0;

};

END_ENGINE