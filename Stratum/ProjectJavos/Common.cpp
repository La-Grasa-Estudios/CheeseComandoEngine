#include "Common.h"

std::string Funkin::GenerateAssetPath(const std::string& prefix, const std::string& file, const std::string& extension)
{
    return std::string(prefix).append(file).append(".").append(extension);
}
