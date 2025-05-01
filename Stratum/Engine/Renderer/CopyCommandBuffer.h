#pragma once

#include "znmsp.h"

#include "RendererContext.h"
#include "RenderCommands.h"

BEGIN_ENGINE

namespace Render {

	class CopyCommandBuffer
	{
	public:

		CopyCommandBuffer();

		void Begin();
		void End();
		void Submit();
		void Wait();

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		nvrhi::CommandListHandle mCommandList;
		nvrhi::EventQueryHandle mEventQuery;

	};
}

END_ENGINE