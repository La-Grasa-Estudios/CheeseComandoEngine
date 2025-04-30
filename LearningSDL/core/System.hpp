#pragma once
#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <set>
#include "Entity.hpp"

class System
{
public:
	std::set<Entity> mEntities;
};




#endif //!SYSTEM_HPP+