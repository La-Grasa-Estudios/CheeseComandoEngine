#pragma once
#include "..\core\System.hpp"
class AnimationSystem : public System 
{
public:
	void Init();
	void Update(float deltaTime);
};