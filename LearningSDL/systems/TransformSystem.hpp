#pragma once
#include "..\core\System.hpp"
class TransformSystem : public System
{
public:
	void Init();
	void Update(float deltaTime);
};