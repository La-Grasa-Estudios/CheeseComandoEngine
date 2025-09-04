#pragma once

#include "znmsp.h"

#include <Util/ThreadSafeQueue.h>

#include <Entity/Components.h>
#include <cstdint>

BEGIN_ENGINE

class Scene;

/// <summary>
/// Used for async resource loading
/// </summary>
class AssetManager
{
public:
	AssetManager(Scene* scene);
	~AssetManager();

	void LoadTextureAsync(const std::string_view& path, uint32_t* pDst);
	void LoadModelAsync(const std::string_view& path, ECS::edict_t entity);

	void Wait();
private:

	void Update();

	std::atomic_bool m_StopLoadingThread;
	std::atomic_bool m_LoadingThreadJoin;

	struct TextureRequest
	{
		std::string path;
		uint32_t* pDst;
	};

	struct ModelRequest
	{
		std::string path;
		ECS::edict_t entity;
	};

	ThreadSafeQueue<TextureRequest> m_TextureRequests;
	ThreadSafeQueue<ModelRequest> m_ModelRequests;

	Scene* m_Scene;
};

END_ENGINE