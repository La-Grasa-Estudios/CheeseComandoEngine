#pragma once

#include "Scene.h"
#include "RendererCommon.h"

#include <stack>

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
}

struct RenderInstance
{
	uint32_t VertexBufferIndex;
	uint32_t IndexBufferIndex;
	uint32_t IndexOffset;
	uint32_t IndexCount;
	uint32_t InstanceIndex;
};

/// <summary>
/// Simple class designed to hold instances that passed the visibility test
/// </summary>
struct RenderQueue
{
	std::vector<RenderInstance> renderInstances;

	void Clear();

	void Push(const RenderInstance& instance);
	void Push(uint32_t VertexBufferIndex, uint32_t IndexBufferIndex, uint32_t IndexOffset, uint32_t IndexCount, uint32_t InstanceIndex);

	RenderQueue() = default;

	/// <summary>
	/// Copies the other instance, uses std::move to avoid memory allocation
	/// </summary>
	RenderQueue(RenderQueue&& other) noexcept;
};

struct Visibility
{
	RenderQueue visQueue;
	void CullQueue(const ViewPose& viewPose, RenderQueue& queue);
};

class Renderer3D
{
public:

	void PushPose(const ViewPose& pose);
	void PopPose();

	ViewPose* GetTopPose();

	void PreRender(Scene* scene);
	void Render(Scene* scene, Render::GraphicsCommandBuffer* pCmdBuffer);

	void BindGraphicsCommonResources(Render::GraphicsCommandBuffer* pCmdBuffer);

	RenderQueue mainVisRenderQueue;

private:

	std::stack<ViewPose> mViewPoses;

};

END_ENGINE