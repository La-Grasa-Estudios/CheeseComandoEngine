#pragma once
#include "..\core\System.hpp"
#include <SDL.h>
//fix
class RenderSystem : public System
{
public:
	void Init();
	void Update(SDL_Renderer* renderer);
};
