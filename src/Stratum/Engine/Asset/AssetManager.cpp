#include "AssetManager.h"

#include <Scene/Scene.h>
#include <thread>

using namespace ENGINE_NAMESPACE;

AssetManager::AssetManager(Scene* scene)
{
	m_Scene = scene;

	// Create background spinning thread that processes all incoming requests

	m_StopLoadingThread = false;
	m_LoadingThreadJoin = false;

	std::thread t([this]{

		while (!m_StopLoadingThread)
		{
			Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(4));
		}
		m_LoadingThreadJoin = true;

	});
}

AssetManager::~AssetManager()
{
	m_StopLoadingThread = true;
	while (!m_LoadingThreadJoin) std::this_thread::yield();
}

void AssetManager::LoadTextureAsync(const std::string_view& path, uint32_t* pDst)
{
}

void AssetManager::LoadModelAsync(const std::string_view& path, ECS::edict_t entity)
{
}

void AssetManager::Wait()
{
	while (!m_TextureRequests.Empty() || !m_ModelRequests.Empty())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void AssetManager::Update()
{
	if (!m_TextureRequests.Empty())
	{
		TextureRequest request = m_TextureRequests.Dequeue();
		auto handle = m_Scene->Resources.LoadTextureImage(request.path);
		*request.pDst = handle;
	}
}
