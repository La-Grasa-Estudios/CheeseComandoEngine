#pragma once

#include "Entity/ECS.h"

#include "SceneResources.h"
#include "Renderer/BindlessDescriptorTable.h"

BEGIN_ENGINE

class Scene;

/// <summary>
/// Represents a system that needs updating every frame
/// Override the following methods to interact with the scene
/// void Update()
/// void PostUpdate()
/// void Init()
/// void RenderImGui() - optional -
/// </summary>
class ISceneSystem
{
public:
	friend Scene;

	virtual ~ISceneSystem() {};

	virtual void Init(Scene* scene) = 0;
	virtual void Update(Scene* scene) = 0;
	virtual void PostUpdate(Scene* scene) = 0;
	virtual void RenderImGui(Scene* scene) {};
private:
	bool mInitialized = false;
};

class AudioEngine;
class Renderer3D;

namespace Internal
{
	class Window;
}

class Scene
{

public:

	Scene();
	~Scene();

	void InitBindlessTable(nvrhi::IBindingLayout* bindingLayout);
	void UpdateSystems();
	void PostUpdate();
	void RenderImGui();

	ECS::EntityManager EntityManager;

	ECS::ComponentManager<NameComponent> Names;
	ECS::ComponentManager<TransformComponent> Transforms;
	ECS::ComponentManager<MeshRendererComponent> Renderers;
	ECS::ComponentManager<SpriteRendererComponent> SpriteRenderers;
	ECS::ComponentManager<SpriteAnimator> SpriteAnimators;
	ECS::ComponentManager<GuiAnchorComponent> GuiAnchors;

	void LoadModel(const std::string& path, const ECS::edict_t edict);

	SceneResources Resources;
	Ref<Render::BindlessDescriptorTable> BindlessTable;

	void RegisterCustomComponent(ECS::ComponentManager_Interface* pInterface, const std::string& name);
	void RegisterCustomSystem(ISceneSystem* pSystem, bool initImmediately = false);

	void SwapScene(Scene* scene);

	template<typename T>
	ECS::ComponentManager<T>* GetComponentManager(const std::string& name)
	{
		if (!mCustomComponents.contains(name))
			return NULL;
		return reinterpret_cast<ECS::ComponentManager<T>*>(mCustomComponents[name]);
	}

	glm::vec2 VirtualScreenSize = {};

	AudioEngine* AudioEngine;
	Renderer3D* RenderPath3D;

	Internal::Window* Window;
	
	Scene* NextScenePtr = 0;

private:

	void UpdateTransforms();
	void UpdateAnimators();
	void UpdateGuiAnchors();

	std::unordered_map<std::string, ECS::ComponentManager_Interface*> mCustomComponents;
	std::vector<ISceneSystem*> mSystems;

};

END_ENGINE