#include "RenderSystem.hpp"
#include "..\core\Coordinator.hpp"
#include "..\components\RenderableComponent.hpp"
#include "..\components\TransformComponent.hpp"
#include "..\ResourceManager.hpp"

#include<iostream>

extern Coordinator gCoordinator;

void RenderSystem::Init()
{

}
//fix
void RenderSystem::Update(SDL_Renderer* renderer)
{
	for (Entity entity : mEntities)
	{
		auto renderable = gCoordinator.GetComponent<Renderable>(entity);
		auto transform = gCoordinator.GetComponent<Transform>(entity);

		SDL_Rect dstRect;

		dstRect.x = static_cast<int>(transform.position.x);
		dstRect.y = static_cast<int>(transform.position.y);
		dstRect.w = static_cast<int>(transform.scale.x * renderable.srcRect.w);
		dstRect.h = static_cast<int>(transform.scale.y * renderable.srcRect.h);

		SDL_Texture* texture = ResourceManager::get_resource_manager().get_texture(renderer, renderable.texture_path);
		
		SDL_SetTextureColorMod(texture, renderable.color_mod.r, renderable.color_mod.g, renderable.color_mod.b);

		SDL_RenderCopyEx(renderer, texture, &renderable.srcRect, &dstRect, renderable.angle, &renderable.origin, renderable.flip);

	}
}