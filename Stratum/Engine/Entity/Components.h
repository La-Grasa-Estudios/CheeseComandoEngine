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

	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
	glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

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
	glm::vec2 Rotation = glm::vec2(0.0f);

	int32_t TextureHandle = -1;

};

struct SpriteAnimator
{
	struct AnimationFrame
	{
		SpriteRendererComponent::SpriteRect Rect;
		glm::ivec2 Offset;
	};

	struct Animation
	{
		std::vector<AnimationFrame> rects;
		uint32_t FrameIndex = 0;
		float FrameRate = 1.0f;
		float Accumulator = 0.0f;
		bool IsLooping = false;
		bool PlayOnIdle = false;
		bool TransitionToDefault = true;
		std::string NextState;

		Animation& SetFrameRate(float fps)
		{
			FrameRate = fps;
			return *this;
		}

		Animation& SetLoop(bool loop)
		{
			IsLooping = loop;
			return *this;
		}

		Animation& SetTransitionToDefault(bool transition)
		{
			TransitionToDefault = transition;
			return *this;
		}

		Animation& SetAnimateOnIdle(bool idle)
		{
			PlayOnIdle = idle;
			return *this;
		}

		Animation& SetNextState(const std::string& state)
		{
			NextState = state;
			return *this;
		}

		Animation& SetFrames(std::vector<AnimationFrame> rects)
		{
			this->rects = rects;
			return *this;
		}
	};

	std::string CurrentAnimation = "";
	std::string DefaultAnimation = "";

	std::unordered_map<std::string, Animation> AnimationMap;

	void SetState(const std::string& state)
	{
		CurrentAnimation = state;
		if (auto a = AnimationMap.find(state); a != AnimationMap.end())
		{
			if (!a->second.PlayOnIdle)
			{
				a->second.Accumulator = 0.0f;
				a->second.FrameIndex = 0;
			}
		}
	}

	AnimationFrame GetCurrentRect()
	{
		if (CurrentAnimation.empty())
			return {};

		if (auto a = AnimationMap.find(CurrentAnimation); a != AnimationMap.end())
		{
			return a->second.rects[a->second.FrameIndex];
		} 

		return {};
	}
};

END_ENGINE