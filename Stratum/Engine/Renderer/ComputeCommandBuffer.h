#pragma once

#include "znmsp.h"

#include "RendererContext.h"
#include "RenderCommands.h"
#include "TextureSampler.h"
#include "Texture3D.h"
#include "ComputePipeline.h"
#include "Buffer.h"

BEGIN_ENGINE

namespace Render {

	class GraphicsCommandBuffer;

	class ComputeCommandBuffer
	{
	public:

		ComputeCommandBuffer(bool async = false);
		ComputeCommandBuffer(GraphicsCommandBuffer* pCmdBuffer);

		void Begin();
		void End();
		void Submit();
		void Wait();
		void ClearState();

		void SetComputePipeline(ComputePipeline* pipeline);

		void SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot);

		void SetTextureResource(ImageResource* texture, uint32_t slot);
		void SetTextureResource(Texture3D* texture, uint32_t slot);
		void SetTextureSampler(TextureSampler* sampler, uint32_t slot);
		void SetBufferResource(Buffer* buffer, uint32_t slot);

		void SetBufferCompute(Buffer* buffer, uint32_t slot);
		void SetTextureCompute(ImageResource* texture, uint32_t slot);
		void SetTextureCompute(Texture3D* texture, uint32_t slot);

		void PushConstants(void* ptr, size_t size);

		void Dispatch(uint32_t x, uint32_t y, uint32_t z);

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		void Barrier(ImageResource* resource);
		void Barrier(Texture3D* resource);
		void Barrier(Buffer* resource);

		void CommitBarriers();

		void ToggleAutomaticBarrierPlacement();

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		void UpdateComputeState();

		nvrhi::CommandListHandle mCommandList;
		nvrhi::IComputePipeline* mSetComputePipeline;

		nvrhi::BindingSetHandle mBindingSet;

		nvrhi::EventQueryHandle mEventQuery;

		std::array<nvrhi::IBuffer*, 32> mBufferUnits;
		std::array<nvrhi::ITexture*, 32> mTextureUnits;
		std::array<nvrhi::ISampler*, 32> mSamplerUnits;
		std::array<nvrhi::IBuffer*, 32> mConstantBuffers;

		std::array<nvrhi::ITexture*, 32> mTextureCompute;
		std::array<nvrhi::IBuffer*, 32> mBufferCompute;

		bool mComputeStateDirty = true;
		bool mBindingStateDirty = true;
		bool mCommitedAnyConstantBuffer = false;
		bool mAutomaticBarriers = true;
		bool mIsOwner = true;

		ComputePipeline* mCurrentPipeline;
	};
}

END_ENGINE