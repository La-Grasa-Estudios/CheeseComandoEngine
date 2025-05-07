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

void Renderer3D::PushPose(const ViewPose& pose)
{
	mViewPoses.push(pose);
}

void Renderer3D::PopPose()
{
	mViewPoses.pop();
}

ViewPose* Renderer3D::GetTopPose()
{
	if (!mViewPoses.empty())
	{
		return &mViewPoses.top();
	}
	return nullptr;
}

void Renderer3D::PreRender(Scene* scene)
{
	
	auto view = GetTopPose();

	if (!view)
		return;

	Render::Frustum frustum(view->ProjectionViewMatrix);

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

void Renderer3D::Render(Scene* scene, Render::GraphicsCommandBuffer* pCmdBuffer)
{
	for (auto& instance : mainVisRenderQueue.renderInstances)
	{
		int32_t indices[3]
		{
			instance.VertexBufferIndex,
			instance.IndexBufferIndex,
			instance.IndexOffset
		};
		pCmdBuffer->PushConstants(indices, sizeof(indices));
		pCmdBuffer->Draw(instance.IndexCount, 0);
	}
}

void Visibility::CullQueue(const ViewPose& viewPose, RenderQueue& queue)
{

}
