#pragma once
#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP
#include "..\math\LadyMath.hpp"
#include <bitset>

using ComponentType = std::uint8_t;

const ComponentType MAX_COMPONENTS = 255;

using Signature = std::bitset<MAX_COMPONENTS>;



#endif //!COMPONENTS_HPP