#pragma once

#include "Scene.h"
#include "RendererCommon.h"

#include "Post/PostProcessingStack.h"

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

class Renderer2D;

class Renderer3D
{
public:

	Renderer3D();

	void SetScene(Scene* scene);
	void SetViewPose(const ViewPose& pose);

	void PreRender(Scene* scene);
	void Render(Scene* scene, Render::Framebuffer* pOutput);

	void BindGraphicsCommonResources(Render::GraphicsCommandBuffer* pCmdBuffer);

	RenderQueue mainVisRenderQueue;
	Render::PostProcessingStack PostProcessingStack;

private:

	void InitializePipelines();
	void ResizeRenderBuffers(glm::ivec2 renderResolution);

	Render::GraphicsCommandBuffer mCommandBuffer;

	Ref<Render::GraphicsPipeline> mStaticOpaquePipeline;
	Ref<Render::GraphicsPipeline> mDynamicOpaquePipeline;

	Ref<Render::ConstantBuffer> mPerFrameCB;

	bool mRequiresGBufferRenderState = false;
	Ref<Render::ImageResource> mRtColorBuffer;
	Ref<Render::ImageResource> mRtNormalBuffer;
	Ref<Render::ImageResource> mRtRoughnessMetalness;
	Ref<Render::ImageResource> mRtDepthBuffer;

	Ref<Render::Framebuffer> mRtGbuffer;

	Ref<Renderer2D> mRenderPath2D;

	ViewPose mViewPose;

	nvrhi::BindingLayoutHandle mBindlessLayout = NULL;

};

END_ENGINE