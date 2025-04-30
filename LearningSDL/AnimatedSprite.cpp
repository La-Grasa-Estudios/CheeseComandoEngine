#include "AnimatedSprite.hpp"
#include "ResourceManager.hpp"
AnimatedSprite::AnimatedSprite(SDL_Renderer* render, std::string path)
{
	SDL_Surface* surface = ResourceManager::get_resource_manager().get_surface(path);
	texture = SDL_CreateTextureFromSurface(render, surface);
	hitboxR.w = 36;
	hitboxR.h = 73;
}

AnimatedSprite::~AnimatedSprite()
{

}

void AnimatedSprite::portion(int x, int y, int w, int h)
{
	destR.x = x;
	destR.y = y;
	destR.w = w;
	destR.h = h;

	hitboxR.x = destR.x + 50;
	hitboxR.y = destR.y + 56;
}

void AnimatedSprite::play_frame(int x, int y, int w, int h, int frame)
{
	sourceR.x = x + w * frame;
	sourceR.y = y;
	sourceR.w = w;
	sourceR.h = h;

}

void AnimatedSprite::render(SDL_Renderer* render, SDL_RendererFlip flip)
{
	SDL_RenderCopyEx(render, texture, &sourceR, &destR, NULL, NULL, flip);
	SDL_SetRenderDrawColor(render, 0, 0, 200,255);
	//SDL_RenderDrawRect(render, &destR);
	SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
	//SDL_RenderDrawRect(render, &hitboxR);
	
}

SDL_bool AnimatedSprite::is_coliding_with(TexturedRectangle& obj)
{
	return SDL_HasIntersection(&hitboxR, obj.get_rect());
}

SDL_Rect* AnimatedSprite::get_rect()
{
	return &destR;
}


SDL_Rect* AnimatedSprite::get_hitbox()
{
	return &hitboxR;
}