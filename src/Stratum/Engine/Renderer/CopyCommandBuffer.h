#pragma once

#include "znmsp.h"

#include "RendererContext.h"
#include "RenderCommands.h"
#include "Buffer.h"

#include <unordered_set>

BEGIN_ENGINE

namespace Render {

	// Command buffer who's only use is to copy data to the gpu
	class CopyCommandBuffer
	{
	public:

		CopyCommandBuffer();

		void Begin();
		void End();
		void Submit();

		void WaitForExecution(CommandListQueueInstance queueInstance, CommandQueue queue);
		CommandListQueueInstance GetQueueExecutionInstance() const;
		/// Do not use unless you wan't to batch commandlists with RendererContext::GetDevice()
		void SetQueueInstance(CommandListQueueInstance queue); 
		void TriggerWaitOnExecutionQueue(CommandQueue queue);

		void SignalEvent(nvrhi::EventQueryHandle event);
		void WriteBuffer(Buffer* buffer, void* data, size_t dataSize, size_t destOffset = 0);

		// Vulkan specific barriers, does not work on D3D12
		void RequireTextureState(ImageResource* pImage, ResourceState before, ResourceState after, nvrhi::TextureSubresourceSet subResources = nvrhi::AllSubresources);
		void RequireBufferState(Buffer* pBuffer, ResourceState before, ResourceState after);
		void CommitBarriers();

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		std::unordered_set<uintptr_t> mTrackedResources;

		uint64_t mCmdInstance = 0;
		nvrhi::CommandListHandle mCommandList;

	};
}

END_ENGINE