#pragma once
#ifndef ANIMATEDSPRITE_HPP
#define ANIMATEDSPRITE_HPP
#include <SDL.h>
#include "ResourceManager.hpp"
#include "TexturedRectangle.hpp"
class AnimatedSprite
{
public:
	AnimatedSprite(SDL_Renderer* render, std::string path);
	~AnimatedSprite();
	void portion(int x, int y, int w, int h);
	void play_frame(int x, int y, int w, int h, int frame);
	void render(SDL_Renderer* render, SDL_RendererFlip flip);
	SDL_Rect* get_rect();
	SDL_Rect* get_hitbox();
	SDL_bool is_coliding_with(TexturedRectangle& obj);
private:
	SDL_Texture* texture;
	SDL_Rect sourceR;
	SDL_Rect destR;
	SDL_Rect hitboxR;
};

#endif //ANIMATEDSPRITE_HPP