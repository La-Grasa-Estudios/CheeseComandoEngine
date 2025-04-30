#pragma once
#ifndef RESOURCEMANAGER_HPP
#define RESOURCEMANAGER_HPP
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <unordered_map>
#include <string>
class ResourceManager
{
private:
	ResourceManager();
	~ResourceManager();
	std::unordered_map<std::string, SDL_Surface*> surfaces;
	std::unordered_map<std::string, SDL_Texture*> textures;
public:
	static ResourceManager& get_resource_manager();
	SDL_Surface* get_surface(std::string path);
	SDL_Texture* get_texture(SDL_Renderer* renderer, std::string path);
	
};

#endif //RESOURCEMANAGER_HPP