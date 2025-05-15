#pragma once

#include "Entity/ECS.h"

#include "SceneResources.h"
#include "Renderer/BindlessDescriptorTable.h"

BEGIN_ENGINE

class Scene
{

public:

	Scene();
	~Scene();

	void InitBindlessTable(nvrhi::IBindingLayout* bindingLayout);

	ECS::EntityManager EntityManager;

	ECS::ComponentManager<NameComponent> Names;
	ECS::ComponentManager<TransformComponent> Transforms;
	ECS::ComponentManager<MeshRendererComponent> Renderers;
	ECS::ComponentManager<SpriteRendererComponent> SpriteRenderers;

	void LoadModel(const std::string& path, const ECS::edict_t edict);

	SceneResources Resources;
	Ref<Render::BindlessDescriptorTable> BindlessTable;

private:


};

END_ENGINE