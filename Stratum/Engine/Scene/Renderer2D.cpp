#include "Renderer2D.h"

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ConstantBuffer.h"
#include <Core/EngineStats.h>

#include "SpriteBatch.h"


using namespace ENGINE_NAMESPACE;

Renderer2D::Renderer2D()
{
	mPerFrameData = CreateRef<Render::ConstantBuffer>(sizeof(PerFrameData));
	mCmdBuffer = CreateRef<Render::GraphicsCommandBuffer>();

	Render::PipelineDescription pipelineDesc{};

	pipelineDesc.ShaderPath = "shaders/2d/2d_sprite.cso";
	pipelineDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(SpriteBatch::SpriteRenderable)));

	pipelineDesc.VertexLayout.VertexAttributes.push_back({ Render::VertexType::FLOAT2_32, Render::VertexInputRate::PER_VERTEX, 0, 0, 0, false });
	pipelineDesc.VertexLayout.Stride = sizeof(glm::vec2);

	pipelineDesc.RasterizerState.CullMode = Render::RCullMode::NOT;
	pipelineDesc.RasterizerState.DepthTest = false;
	pipelineDesc.StencilState.DepthEnable = false;

	pipelineDesc.BlendState.BlendStates[0].DestBlend = Render::Blend::INV_SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].SrcBlend = Render::Blend::SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].DestBlendAlpha = Render::Blend::ONE;
	pipelineDesc.BlendState.BlendStates[0].SrcBlendAlpha = Render::Blend::SRC_ALPHA;
	pipelineDesc.BlendState.BlendStates[0].EnableBlend = true;

	mMainPipeline = CreateRef<Render::GraphicsPipeline>(pipelineDesc);

	Render::TextureSamplerDescription samplerDesc{};

	mBilinearSampler = CreateRef<Render::TextureSampler>(samplerDesc);
}

void Renderer2D::PreRender(Scene* scene)
{
	Z_PROFILE_SCOPE("Renderer2D::PreRender");

	if (!mSpriteBatch)
		mSpriteBatch = CreateRef<SpriteBatch>(&scene->Resources);

	auto& entities = scene->SpriteRenderers.GetEntities();

	for (int i = 0; i < 8; i++)
		mRenderQueues[i].Clear();

	for (auto entity : entities)
	{

		if (!scene->Transforms.HasComponent(entity))
			continue;

		auto& transform = scene->Transforms.Get(entity);
		auto& renderer = scene->SpriteRenderers.Get(entity);

		RenderQueue2D::RenderInstance instance{};

		instance.center = renderer.Center;
		instance.rect = renderer.Rect;
		instance.texture = renderer.TextureHandle;
		instance.transform = transform.ModelMatrix;

		if (scene->SpriteAnimators.HasComponent(entity))
		{
			instance.rect = scene->SpriteAnimators.Get(entity).GetCurrentRect().Rect;
			instance.transform = glm::translate(instance.transform, glm::vec3(-scene->SpriteAnimators.Get(entity).GetCurrentRect().Offset, 0.0f));
		}

		instance.transform = glm::rotate(instance.transform, glm::radians(renderer.Rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));

		mRenderQueues[renderer.RenderLayer].Push(instance);
	}

}

void Renderer2D::Render(Scene* scene, Render::Framebuffer* pOutput)
{
	Z_PROFILE_SCOPE("Renderer2D::Render");

	mSpriteBatch->Begin();

	if (mMainPipeline->ShaderDesc.RenderTarget != pOutput)
		mMainPipeline->SetRenderTarget(pOutput);

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < mRenderQueues[i].instances.size(); j++)
		{
			auto& instance = mRenderQueues[i].instances[j];

			mSpriteBatch->DrawSprite(instance.transform, instance.rect, instance.center, instance.texture);
		}
	}

	glm::vec2 size = glm::vec2(pOutput->GetSize()) / 3.0f;

	glm::mat4 proj = glm::ortho(-size.x, size.x, -size.y, size.y);
	glm::mat4 view = glm::identity<glm::mat4>();

	glm::mat4 projView = proj * view;

	Render::Viewport viewport{};

	viewport.width = pOutput->GetSize().x;
	viewport.height = pOutput->GetSize().y;

	mCmdBuffer->Begin();

	mCmdBuffer->RequireFramebufferState(pOutput, Render::ResourceState::Present, Render::ResourceState::RenderTarget);

	mCmdBuffer->ClearBuffer(pOutput, 0, glm::vec4(0.0f));

	mCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), &projView);
	mCmdBuffer->CommitBarriers();
	mCmdBuffer->SetFramebuffer(pOutput);
	mCmdBuffer->SetPipeline(mMainPipeline.get());
	mCmdBuffer->SetConstantBuffer(mPerFrameData.get(), 1);
	mCmdBuffer->SetViewport(&viewport);
	mCmdBuffer->SetTextureSampler(mBilinearSampler.get(), 0);
	mCmdBuffer->SetBindlessDescriptorTable(scene->BindlessTable);

	mSpriteBatch->End(mCmdBuffer.get());

	mCmdBuffer->RequireFramebufferState(pOutput, Render::ResourceState::RenderTarget, Render::ResourceState::Present);
	mCmdBuffer->End();
}

void Renderer2D::Submit()
{
	mCmdBuffer->Submit();
}
