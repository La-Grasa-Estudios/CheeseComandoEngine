#pragma once
#ifndef TEXTUREDRECTANGLE_HPP
#define TEXTUREDRECTANGLE_HPP
#include <SDL.h>
#include <string>
class TexturedRectangle
{
private:
	SDL_Rect rectangle;
	std::string path;
	SDL_Renderer* render = nullptr;
	SDL_Texture* texture = nullptr;
public:
	TexturedRectangle(SDL_Renderer* renderer, std::string file);
	~TexturedRectangle();

	void set_attributes(int X, int Y, int W, int H);
	void set_texture();
	void set_render_copy();
	SDL_Rect* get_rect();
	SDL_bool is_coliding_with(TexturedRectangle& obj);

};

#endif //TEXTUREDRECTANGLE_HPP