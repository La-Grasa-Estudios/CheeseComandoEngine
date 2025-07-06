#pragma once

#include "znmsp.h"

#include <format>

BEGIN_ENGINE

namespace Utils
{
	std::wstring ToWideString(const std::string& str);

	template<typename ... Args>
	static auto FormatString(std::wstring_view fmt, Args&&... args) {
		return std::vformat(fmt, std::make_wformat_args(args...));
	}
}

END_ENGINE