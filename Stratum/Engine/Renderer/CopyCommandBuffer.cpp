#include "CopyCommandBuffer.h"

using namespace ENGINE_NAMESPACE;

Render::CopyCommandBuffer::CopyCommandBuffer()
{
	nvrhi::CommandListParameters params{};
	params.queueType = nvrhi::CommandQueue::Copy;

	if (RendererContext::get_api() == RendererAPI::DX11)
	{
		params.queueType = nvrhi::CommandQueue::Graphics;
	}

	mCommandList = RendererContext::GetDevice()->createCommandList(params);
}

void Render::CopyCommandBuffer::Begin()
{
	mCommandList->setEnableAutomaticBarriers(false);
	mCommandList->open();
}

void Render::CopyCommandBuffer::End()
{
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

void Render::CopyCommandBuffer::UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data)
{
	mCommandList->writeBuffer(pBuffer->GetHandle(), data, pBuffer->Size);
}

nvrhi::ICommandList* Render::CopyCommandBuffer::GetNativeCommandList()
{
	mCommandList->commitBarriers();
	return mCommandList;
}
