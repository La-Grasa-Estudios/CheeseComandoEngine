#include "Renderer3D.h"

#include "Core/JobManager.h"

#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/Frustum.h"

#include "Scene/Scene.h"


using namespace ENGINE_NAMESPACE;

void RenderQueue::Clear()
{
	this->renderInstances.clear();
}

void RenderQueue::Push(const RenderInstance& instance)
{
	renderInstances.push_back(instance);
}

void RenderQueue::Push(uint32_t VertexBufferIndex, uint32_t IndexBufferIndex, uint32_t IndexOffset, uint32_t IndexCount, uint32_t InstanceIndex)
{
	renderInstances.emplace_back(VertexBufferIndex, IndexBufferIndex, IndexOffset, IndexCount, InstanceIndex);
}

RenderQueue::RenderQueue(RenderQueue&& other) noexcept
{
	renderInstances = std::move(other.renderInstances);
}

Renderer3D::Renderer3D()
{
	using namespace Render;

	mCommandBuffer = {};

	InitializePipelines();
}

void Renderer3D::SetScene(Scene* scene)
{
	scene->InitBindlessTable(mDynamicOpaquePipeline->BindingLayout);
}

void Renderer3D::SetViewPose(const ViewPose& pose)
{
	mViewPose = pose;
}

void Renderer3D::PreRender(Scene* scene)
{

	Render::Frustum frustum(mViewPose.ProjectionViewMatrix);

	mainVisRenderQueue.Clear();

	auto& entities = scene->Renderers.GetEntities();

	uint32_t instanceIndex = 0;

	for (auto edict : entities)
	{
		auto& renderer = scene->Renderers.Get(edict);

		for (auto& subset : renderer.Subsets)
		{
			mainVisRenderQueue.Push(renderer.VertexBufferDescriptorIndex, renderer.IndexBufferDescriptorIndex, subset.IndexOffset, subset.IndexCount, instanceIndex++);
		}

	}

}

void Renderer3D::Render(Scene* scene, Render::Framebuffer* pOutput)
{
	using namespace Render;

	ResizeRenderBuffers(pOutput->GetSize());

	Render::Viewport viewport{};
	viewport.width = mRtColorBuffer->GetSize().x;
	viewport.height = mRtColorBuffer->GetSize().y;

	glm::mat4 model = glm::identity<glm::mat4>();

	glm::mat4 mvp = mViewPose.ProjectionViewMatrix * model;

	mCommandBuffer.Begin();

	mCommandBuffer.UpdateConstantBuffer(mPerFrameCB.get(), &mvp);

	mCommandBuffer.SetViewport(&viewport);
	mCommandBuffer.SetPipeline(mDynamicOpaquePipeline.get());

	mCommandBuffer.RequireTextureState(mRtColorBuffer.get(), ResourceState::ShaderResource, ResourceState::RenderTarget);
	mCommandBuffer.RequireTextureState(mRtNormalBuffer.get(), ResourceState::ShaderResource, ResourceState::RenderTarget);
	mCommandBuffer.RequireTextureState(mRtRoughnessMetalness.get(), ResourceState::ShaderResource, ResourceState::RenderTarget);
	mCommandBuffer.RequireTextureState(mRtDepthBuffer.get(), ResourceState::ShaderResource, ResourceState::DepthWrite);

	mCommandBuffer.SetFramebuffer(mRtGbuffer.get());
	mCommandBuffer.ClearDepth(mRtGbuffer.get(), 1.0f);

	mCommandBuffer.SetConstantBuffer(mPerFrameCB.get(), 1);
	mCommandBuffer.SetBindlessDescriptorTable(scene->BindlessTable);

	for (auto& instance : mainVisRenderQueue.renderInstances)
	{
		int32_t indices[3]
		{
			instance.VertexBufferIndex,
			instance.IndexBufferIndex,
			instance.IndexOffset
		};
		mCommandBuffer.PushConstants(indices, sizeof(indices));
		mCommandBuffer.Draw(instance.IndexCount, 0);
	}

	mCommandBuffer.RequireTextureState(mRtColorBuffer.get(), ResourceState::RenderTarget, ResourceState::ShaderResource);
	mCommandBuffer.RequireTextureState(mRtNormalBuffer.get(), ResourceState::RenderTarget, ResourceState::ShaderResource);
	mCommandBuffer.RequireTextureState(mRtRoughnessMetalness.get(), ResourceState::RenderTarget, ResourceState::ShaderResource);
	mCommandBuffer.RequireTextureState(mRtDepthBuffer.get(), ResourceState::DepthWrite, ResourceState::ShaderResource);

	mCommandBuffer.End();
	mCommandBuffer.Submit();
}

void Renderer3D::InitializePipelines()
{
	using namespace Render;
	
	mPerFrameCB = CreateRef<ConstantBuffer>(sizeof(glm::mat4));

	PipelineDescription description{};

	description.ShaderPath = "shaders/deferred_gbuffer_opaque.cso";
	description.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, 12));

	Render::StaticBindingTable bindingTable =
	{
		nvrhi::BindingSetItem::ConstantBuffer(1, mPerFrameCB->Handle),
	};

	mDynamicOpaquePipeline = CreateRef<GraphicsPipeline>(description);
	mDynamicOpaquePipeline->UpdateStaticBinding(bindingTable);
}

void Renderer3D::ResizeRenderBuffers(glm::ivec2 renderResolution)
{
	using namespace Render;

	bool needsResize = !mRtColorBuffer || mRtColorBuffer->GetSize() != renderResolution;

	if (needsResize)
	{
		ImageDescription imageDesc{};

		imageDesc.Width = renderResolution.x;
		imageDesc.Height = renderResolution.y;
		imageDesc.ClearValue = glm::vec4(0.0f);
		imageDesc.AllowFramebufferUsage = true;
		
		ImageDescription colorDesc = imageDesc;
		colorDesc.Format = ImageFormat::R11G11B10_FLOAT;

		ImageDescription normalDesc = imageDesc;
		normalDesc.Format = ImageFormat::RG16_UNORM;

		ImageDescription roughMetalDesc = imageDesc;
		roughMetalDesc.Format = ImageFormat::RG8_UNORM;

		ImageDescription depthDesc = imageDesc;
		depthDesc.Format = ImageFormat::DEPTH24_S8;
		depthDesc.ClearValue = glm::vec4(1.0f, 255.0f, 0.0f, 0.0f);

		mRtColorBuffer = CreateRef<ImageResource>(colorDesc);
		mRtNormalBuffer = CreateRef<ImageResource>(normalDesc);
		mRtRoughnessMetalness = CreateRef<ImageResource>(roughMetalDesc);
		mRtDepthBuffer = CreateRef<ImageResource>(depthDesc);

		FramebufferDesc fbDesc{};

		fbDesc.Attachments.push_back({ mRtColorBuffer.get() });
		fbDesc.Attachments.push_back({ mRtNormalBuffer.get() });
		fbDesc.Attachments.push_back({ mRtRoughnessMetalness.get() });
		fbDesc.Attachments.push_back({ mRtDepthBuffer.get() });

		mRtGbuffer = CreateRef<Framebuffer>(fbDesc);

		mDynamicOpaquePipeline->SetRenderTarget(mRtGbuffer);

		mRequiresGBufferRenderState = true;
	}
}

void Visibility::CullQueue(const ViewPose& viewPose, RenderQueue& queue)
{

}
