#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "RendererContext.h"
#include "RenderCommands.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "TextureSampler.h"
#include "Texture3D.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include <unordered_set>

BEGIN_ENGINE

namespace Render {

	class BindlessDescriptorTable;

	class GraphicsCommandBuffer {

	public:

		GraphicsCommandBuffer();
		~GraphicsCommandBuffer();

		void Begin();
		void End();
		void Submit();

		void SetPipeline(GraphicsPipeline* pipeline);

		void SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot);
		void SetVertexBuffer(VertexBuffer* buffer, uint32_t slot);
		void SetIndexBuffer(IndexBuffer* buffer);
		void SetFramebuffer(Framebuffer* framebuffer);
		void PushConstants(void* ptr, size_t size);

		void SetTextureResource(ImageResource* texture, uint32_t slot);
		void SetTextureResource(Texture3D* texture, uint32_t slot);
		void SetBufferResource(Buffer* buffer, uint32_t slot);
		void SetTextureSampler(TextureSampler* sampler, uint32_t slot);

		void SetBufferUav(Buffer* buffer, uint32_t slot);
		void SetBindlessDescriptorTable(Ref<BindlessDescriptorTable> table);

		void SetIndirectBuffer(Buffer* buffer);

		void SetViewport(Viewport* vp);

		void Draw(uint32_t count);
		void DrawInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance);
		void DrawIndirect(uint32_t count, uint64_t offsetBytes);

		void DrawIndexed(uint32_t count);
		void DrawIndexedInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance);
		void DrawIndexedIndirect(uint32_t count);

		void ClearBuffer(uint32_t index, const glm::vec4 color);
		void ClearDepth(float depth, uint32_t stencil = 0xFF);

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		void ClearVertexBuffers();

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		void UpdateGraphicsState();

		struct BoundBindlessTable
		{
			uint64_t FrameIndex;
			Ref<BindlessDescriptorTable> Table;
		};

		struct BindlessTableHasher
		{
			std::size_t operator()(const BoundBindlessTable& item) const;
		};

		struct BindlessTableEqual
		{
			bool operator()(const BoundBindlessTable& a, const BoundBindlessTable& b) const;
		};

		nvrhi::CommandListHandle mCommandList;
		nvrhi::IGraphicsPipeline* mSetGraphicsPipeline;
		nvrhi::IFramebuffer* mSetFramebuffer;
		nvrhi::ViewportState mSetViewport;
		nvrhi::IndexBufferBinding mSetIndexBuffer;
		nvrhi::IDescriptorTable* mSetDescriptorTable;

		nvrhi::IBuffer* mIndirectBufferParams;

		nvrhi::BindingSetHandle mBindingSet;

		std::array<nvrhi::ITexture*, 32> mTextureUnits;
		std::array<nvrhi::ISampler*, 32> mSamplerUnits;
		std::array<nvrhi::IBuffer*, 32> mConstantBuffers;
		std::array<nvrhi::IBuffer*, 16> mBufferResources;
		std::array<nvrhi::IBuffer*, 8> mBufferUavs;
		std::array<nvrhi::IBuffer*, nvrhi::c_MaxVertexAttributes> mVertexBuffers;

		bool mGraphicsStateDirty = true;
		bool mBindingStateDirty = true;
		bool mCommitedAnyConstantBuffer = false;

		std::unordered_set<BoundBindlessTable, BindlessTableHasher, BindlessTableEqual> mBoundTables;

		GraphicsPipeline* mCurrentPipeline;
	};

}

END_ENGINE