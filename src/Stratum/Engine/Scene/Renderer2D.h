#pragma once

#include "Scene.h"
#include "RendererCommon.h"

#include "Renderer/ComputeCommandBuffer.h"
#include "Renderer/CopyCommandBuffer.h"

#include "znmsp.h"

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
	class Framebuffer;
	class GraphicsPipeline;
	class ConstantBuffer;
	class TextureSampler;
}

class Renderer2D
{
	struct PerFrameData
	{
		glm::mat4 ProjView[16];
		glm::vec2 ScreenSize;
	};

public:

	Renderer2D();

	void PreRender(Scene* scene, Render::Framebuffer* pOutput);
	void Render(Scene* scene, Render::Framebuffer* pOutput);
	void Submit();

	void UpdateScreenSize(const glm::ivec2& size);

	glm::vec2 VirtualScreenSize = {};

	void SetConstantBuffer(Render::ConstantBuffer* pBuffer, uint32_t slot);

	Render::Framebuffer* GetRenderTarget();

private:

	struct ConstantBufferSlot
	{
		Render::ConstantBuffer* pBuffer;
		uint32_t Slot;
	};

	Ref<SpriteBatch> mSpriteBatch;
	Ref<Render::GraphicsPipeline> mMainPipeline;

	Ref<Render::GraphicsCommandBuffer> mCmdBuffer;
	Ref<Render::ComputeCommandBuffer> mComputeCmdBuffer;
	Ref<Render::CopyCommandBuffer> mCopyCmdBuffer;
	Ref<Render::ConstantBuffer> mPerFrameData;

	Ref<Render::TextureSampler> mBilinearSampler;
	Ref<Render::TextureSampler> mNearestSampler;

	Ref<Render::TextureSampler> mBilinearClampSampler;
	Ref<Render::TextureSampler> mNearestClampSampler;

	Ref<Render::ImageResource> mColorBufferRT;
	Ref<Render::Framebuffer> mMainRenderTarget;

	std::array<RenderQueue2D, 16> mRenderQueues;
	std::array<ECS::edict_t, 16> mCameras;

	std::vector<ConstantBufferSlot> mCbuffers;

};

END_ENGINE