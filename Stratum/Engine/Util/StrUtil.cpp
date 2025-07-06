#include "StrUtil.h"

using namespace ENGINE_NAMESPACE;

std::wstring Utils::ToWideString(const std::string& str)
{
    std::wstring wide; wchar_t w; mbstate_t mb{};
    size_t n = 0, len = str.length() + 1;
    while (auto res = mbrtowc(&w, str.c_str() + n, len - n, &mb)) {
        if (res == size_t(-1) || res == size_t(-2))
            throw "invalid encoding";

        n += res;
        wide += w;
    }
    return wide;
}
