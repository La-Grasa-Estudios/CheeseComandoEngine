#pragma once

#include "znmsp.h"

#include "RendererContext.h"
#include "RenderCommands.h"

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

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		uint64_t mCmdInstance = 0;
		nvrhi::CommandListHandle mCommandList;

	};
}

END_ENGINE