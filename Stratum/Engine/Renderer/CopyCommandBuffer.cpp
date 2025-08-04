#include "CopyCommandBuffer.h"

using namespace ENGINE_NAMESPACE;

Render::CopyCommandBuffer::CopyCommandBuffer()
{
	nvrhi::CommandListParameters params{};
	params.queueType = nvrhi::CommandQueue::Copy;
	params.enableImmediateExecution = false;

	if (RendererContext::get_api() == RendererAPI::DX11)
	{
		params.queueType = nvrhi::CommandQueue::Graphics;
	}

	mCommandList = RendererContext::GetDevice()->createCommandList(params);
}

void Render::CopyCommandBuffer::Begin()
{
	mTrackedResources.clear();
	mCommandList->open();
	mCommandList->setEnableAutomaticBarriers(false);
}

void Render::CopyCommandBuffer::End()
{
	CommitBarriers();
	mCommandList->close();
}

void Render::CopyCommandBuffer::Submit()
{
	mCmdInstance = RendererContext::GetDevice()->executeCommandList(mCommandList, CommandQueue::Copy);
}

void Render::CopyCommandBuffer::WaitForExecution(CommandListQueueInstance queueInstance, CommandQueue queue)
{
	RendererContext::GetDevice()->queueWaitForCommandList(nvrhi::CommandQueue::Copy, queue, queueInstance);
}

Render::CommandListQueueInstance Render::CopyCommandBuffer::GetQueueExecutionInstance() const
{
	return mCmdInstance;
}

void Render::CopyCommandBuffer::SetQueueInstance(CommandListQueueInstance queue)
{
	mCmdInstance = queue;
}

void Render::CopyCommandBuffer::TriggerWaitOnExecutionQueue(CommandQueue queue)
{
	RendererContext::GetDevice()->queueWaitForCommandList(queue, CommandQueue::Copy, mCmdInstance);
}

void Render::CopyCommandBuffer::RequireTextureState(ImageResource* pImage, ResourceState before, ResourceState after, nvrhi::TextureSubresourceSet subResources)
{
	if (RendererContext::get_api() != Render::RendererAPI::VULKAN)
		return;
	if (!mTrackedResources.contains((uintptr_t)pImage->Handle.Get()))
	{
		mTrackedResources.insert((uintptr_t)pImage->Handle.Get());
		mCommandList->beginTrackingTextureState(pImage->Handle, subResources, before);
	}
	mCommandList->setTextureState(pImage->Handle, subResources, after);
}

void Render::CopyCommandBuffer::RequireBufferState(Buffer* pBuffer, ResourceState before, ResourceState after)
{
	if (RendererContext::get_api() != Render::RendererAPI::VULKAN)
		return;
	if (!mTrackedResources.contains((uintptr_t)pBuffer->Handle.Get()))
	{
		mTrackedResources.insert((uintptr_t)pBuffer->Handle.Get());
		mCommandList->beginTrackingBufferState(pBuffer->Handle, before);
	}
	mCommandList->setBufferState(pBuffer->Handle, after);
}

void Render::CopyCommandBuffer::CommitBarriers()
{
	mCommandList->commitBarriers();
}

void Render::CopyCommandBuffer::UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data)
{
	if (RendererContext::get_api() == Render::RendererAPI::VULKAN)
	{
		mCommandList->beginTrackingBufferState(pBuffer->GetHandle(), nvrhi::ResourceStates::ConstantBuffer);
		mCommandList->setBufferState(pBuffer->GetHandle(), nvrhi::ResourceStates::CopyDest);
	}
	mCommandList->writeBuffer(pBuffer->GetHandle(), data, pBuffer->Size);
	if (RendererContext::get_api() == Render::RendererAPI::VULKAN)
	{
		mCommandList->setBufferState(pBuffer->GetHandle(), nvrhi::ResourceStates::ConstantBuffer);
	}
}

nvrhi::ICommandList* Render::CopyCommandBuffer::GetNativeCommandList()
{
	return mCommandList;
}
