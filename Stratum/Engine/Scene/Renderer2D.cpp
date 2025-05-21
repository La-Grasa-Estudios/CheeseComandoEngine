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

	mMainCamera = {};
	mGuiCamera = {};

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
	{
		mRenderQueues[i].Clear();
		mAlphaRenderQueues[i].Clear();
		mGuiRenderQueues[i].Clear();
		mAlphaGuiRenderQueues[i].Clear();
	}

	for (auto entity : entities)
	{

		if (!scene->Transforms.HasComponent(entity))
			continue;

		auto& transform = scene->Transforms.Get(entity);
		auto& renderer = scene->SpriteRenderers.Get(entity);

		if (!renderer.Enabled) continue;

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
		instance.color = renderer.SpriteColor;

		if (!renderer.IsGui)
		{
			if (instance.color.a < 0.99f)
			{
				mAlphaRenderQueues[renderer.RenderLayer].Push(instance);
			}
			else
			{
				mRenderQueues[renderer.RenderLayer].Push(instance);
			}
		}
		else
		{
			if (instance.color.a < 0.99f)
				mAlphaGuiRenderQueues[renderer.RenderLayer].Push(instance);
			else
				mGuiRenderQueues[renderer.RenderLayer].Push(instance);
		}
	}

}

void Renderer2D::Render(Scene* scene, Render::Framebuffer* pOutput)
{
	Z_PROFILE_SCOPE("Renderer2D::Render");


	if (mMainPipeline->ShaderDesc.RenderTarget != pOutput)
		mMainPipeline->SetRenderTarget(pOutput);

	float scaledWidth = pOutput->GetSize().x;
	float scaledHeight = pOutput->GetSize().y;

	int scaleFactor = 1;
	int k = 1000;

	for (; scaleFactor < k && scaledWidth / (scaleFactor + 1) >= 320 && scaledHeight / (scaleFactor + 1) >= 240; scaleFactor++) {}

	scaledWidth = scaledWidth / (float)scaleFactor;
	scaledHeight = scaledHeight / (float)scaleFactor;
	int screenWidth = (int)glm::ceil(scaledWidth);
	int screenHeight = (int)glm::ceil(scaledHeight);

	glm::vec2 size = (glm::vec2(scaledWidth, scaledHeight) / 2.0f) * 9.0f;
	VirtualScreenSize = size;

	Render::Viewport viewport{};

	viewport.width = pOutput->GetSize().x;
	viewport.height = pOutput->GetSize().y;

	mCmdBuffer->Begin();

	mCmdBuffer->RequireFramebufferState(pOutput, Render::ResourceState::Present, Render::ResourceState::RenderTarget);

	mCmdBuffer->ClearBuffer(pOutput, 0, glm::vec4(0.0f));

	mCmdBuffer->CommitBarriers();
	mCmdBuffer->SetFramebuffer(pOutput);
	mCmdBuffer->SetPipeline(mMainPipeline.get());
	mCmdBuffer->SetConstantBuffer(mPerFrameData.get(), 1);
	mCmdBuffer->SetViewport(&viewport);
	mCmdBuffer->SetTextureSampler(mBilinearSampler.get(), 0);
	mCmdBuffer->SetBindlessDescriptorTable(scene->BindlessTable);

	RenderCamera(&mMainCamera, mRenderQueues, scene, pOutput);
	RenderCamera(&mMainCamera, mAlphaRenderQueues, scene, pOutput);

	RenderCamera(&mGuiCamera, mGuiRenderQueues, scene, pOutput);
	RenderCamera(&mGuiCamera, mAlphaGuiRenderQueues, scene, pOutput);

	mCmdBuffer->RequireFramebufferState(pOutput, Render::ResourceState::RenderTarget, Render::ResourceState::Present);
	mCmdBuffer->End();
}

void Renderer2D::Submit()
{
	mCmdBuffer->Submit();
}

void Renderer2D::SetCameraPosition(const glm::vec2& position)
{
	mMainCamera.Position = position;
}

void Renderer2D::SetGuiCameraPosition(const glm::vec2& position)
{
	mGuiCamera.Position = position;
}

void Renderer2D::SetCameraZoom(const glm::vec2& zoom)
{
	mMainCamera.Zoom = zoom;
}

void Renderer2D::SetGuiCameraZoom(const glm::vec2& zoom)
{
	mGuiCamera.Zoom = zoom;
}

void Renderer2D::SetCameraRotation(float rotation)
{
	mMainCamera.Rotation = rotation;
}

void Renderer2D::SetGuiCameraRotation(float rotation)
{
	mGuiCamera.Rotation = rotation;
}

void Renderer2D::RenderCamera(Camera2D* camera, RenderQueue2D* renderQueues, Scene* scene, Render::Framebuffer* pOutput)
{
	mSpriteBatch->Begin();

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < renderQueues[i].instances.size(); j++)
		{
			auto& instance = renderQueues[i].instances[j];

			mSpriteBatch->DrawSprite(instance.transform, instance.rect, instance.center, instance.color, instance.texture);
		}
	}

	glm::mat4 proj = glm::ortho(-VirtualScreenSize.x, VirtualScreenSize.x, -VirtualScreenSize.y, VirtualScreenSize.y);
	glm::mat4 view = glm::identity<glm::mat4>();

	view = glm::translate(view, glm::vec3(-camera->Position, 0.0f));
	view = glm::rotate(view, glm::radians(camera->Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	view = glm::scale(view, glm::vec3(camera->Zoom, 1.0f));

	glm::mat4 projView = proj * view;

	mCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), &projView);
	mCmdBuffer->CommitBarriers();

	mSpriteBatch->End(mCmdBuffer.get());
}
