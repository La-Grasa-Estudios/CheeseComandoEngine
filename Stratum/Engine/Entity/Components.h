#pragma once

#include "znmsp.h"

#include "glm/ext.hpp"

#include "ComponentManager.h"

BEGIN_ENGINE

struct NameComponent
{
	std::string Name;
};

/// <summary>
/// You can get the values directly, its not recommended to set their values directly use the setters, if you do modify them directly set the IsDirty flag to true
/// </summary>
struct TransformComponent
{
	ECS::edict_t Parent = ECS::C_INVALID_ENTITY;

	glm::vec3 Position = glm::vec3(1.0f);
	glm::vec3 Scale;
	glm::quat Rotation;

	glm::mat4 ModelMatrix = glm::identity<glm::mat4>();
	glm::mat4 InverseBindMatrix = glm::identity<glm::mat4>();

	bool IsDirty = true;

	void SetWorldPosition(const glm::vec3& position);
	void SetPosition(const glm::vec3& position);
	void SetScale(const glm::vec3& scale);
	void SetRotation(const glm::quat& rotation);

	glm::quat GetWorldSpaceRotation();
	glm::vec3 GetWorldSpacePosition() const;

};

struct MeshRendererComponent
{
	struct MeshSubset
	{
		int32_t IndexOffset;
		int32_t IndexCount;

		int32_t MaterialIndex = -1;
	};

	std::vector<MeshSubset> Subsets;

	int32_t VertexBufferDescriptorIndex = -1;
	int32_t IndexBufferDescriptorIndex = -1;
};

struct SpriteRendererComponent
{

	struct SpriteRect
	{
		glm::ivec2 position;
		glm::ivec2 size;
	};

	enum SpriteRenderLayer
	{
		LAYER_BG2,
		LAYER_BG1,
		LAYER_BG0,
		LAYER_FG,
		LAYER_FG0,
		LAYER_FG1,
		LAYER_FG2,
	};

	SpriteRenderLayer RenderLayer = LAYER_FG;
	SpriteRect Rect;

	glm::vec2 Center = glm::vec2(0.0f);

	int32_t TextureHandle = -1;

};

END_ENGINE