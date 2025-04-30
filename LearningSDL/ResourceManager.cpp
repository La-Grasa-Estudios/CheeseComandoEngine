#include "ResourceManager.hpp"
#include <iostream>
ResourceManager::ResourceManager()
{

}

ResourceManager::~ResourceManager()
{
	for (auto& key_surface : surfaces)
	{
		SDL_FreeSurface(key_surface.second);
	}
	std::unordered_map<std::string, SDL_Surface*>().swap(surfaces);
	std::cout << "\nresource manager destructor called\n";
}

ResourceManager& ResourceManager::get_resource_manager()
{
	static ResourceManager* instance = new ResourceManager();
	return *instance;
}

SDL_Surface* ResourceManager::get_surface(std::string path)
{
	auto surface = surfaces.find(path);
	if (surface != surfaces.end())
	{
		return surface->second;
	}
	else
	{
		SDL_Surface* surface = IMG_Load(path.c_str());

		surfaces.insert({ path, surface });
		return surfaces[path];
		
	}
	return nullptr;
}

SDL_Texture* ResourceManager::get_texture(SDL_Renderer* renderer, std::string path)
{
	if (textures.find(path) != textures.end()) 
	{
		return textures[path];
		
	}
	else
	{
		SDL_Surface* surface = IMG_Load(path.c_str());
		if (!surface) return nullptr;
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		if (!texture) return nullptr;
		SDL_FreeSurface(surface);
		textures.insert({ path, texture });
		return textures[path];
	}
}
