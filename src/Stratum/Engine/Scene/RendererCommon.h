#pragma once

#include "znmsp.h"
#include "SpriteBatch.h"
#include <Entity/Components.h>

#include <glm/ext.hpp>
#include <algorithm>

BEGIN_ENGINE;

struct ViewPose
{
	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;

	glm::mat4 ProjectionViewMatrix;

	glm::mat4 InverseProjectionMatrix;
	glm::mat4 InverseViewMatrix;
	glm::mat4 InverseProjectionViewMatrix;

	ViewPose() = default;
	ViewPose(const glm::mat4& Proj, const glm::mat4& View);
};

namespace RenderUtil
{
	glm::mat4 GetProjectionMatrix(ECS::edict_t entity, Scene* scene);
	glm::mat4 GetViewMatrix(ECS::edict_t entity, Scene* scene);
}

enum Render2DInstanceKind
{
	SPRITE,
	TEXT,
	SHAPE,
};
struct Render2DInstance
{
	Render2DInstanceKind kind = Render2DInstanceKind::SPRITE;
	union
	{
		SpriteBatch::SpriteInstance batch;
		struct
		{
			uint32_t textEntity;
			void* userData;
		} text;
	};
	uint32_t zIndex;

	constexpr bool operator >(const Render2DInstance& other) const
	{
		return zIndex > other.zIndex;
	}

	constexpr bool operator <(const Render2DInstance& other) const
	{
		return zIndex < other.zIndex;
	}

};

struct RenderQueue2D
{

	

	std::vector<Render2DInstance> instances;

	void Push(const Render2DInstance& instance)
	{
		instances.push_back(instance);
	}

	void Sort()
	{
		std::sort(instances.begin(), instances.end(), std::less<Render2DInstance>());
	}

	void Clear()
	{
		instances.clear();
	}

};

END_ENGINE