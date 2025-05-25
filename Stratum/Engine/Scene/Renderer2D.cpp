#include "Renderer2D.h"

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ConstantBuffer.h"
#include <Core/EngineStats.h>

#include "SpriteBatch.h"

#include "Core/JobManager.h"

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

	mSpriteBatch->SetResources(&scene->Resources);

	mRenderQueue.Clear();
	mGuiRenderQueue.Clear();

	auto& entities = scene->SpriteRenderers.GetEntities();

	struct AABB
	{
		float x0;
		float y0;
		float x1;
		float y1;
		bool Overlap(const AABB& other) const
		{
			return other.x1 > x0 && other.x0 < x1 && other.y1 > y0 && other.y0;
		}
	};

	AABB screenAABB = { -VirtualScreenSize.x, -VirtualScreenSize.y, VirtualScreenSize.x, VirtualScreenSize.y };

	for (auto entity : entities)
	{

		if (!scene->Transforms.HasComponent(entity))
			continue;

		auto& transform = scene->Transforms.Get(entity);
		auto& renderer = scene->SpriteRenderers.Get(entity);

		if (!renderer.Enabled) continue;

		glm::vec2 position = transform.Position;
		glm::vec2 size = glm::vec2(renderer.Rect.size) * glm::vec2(transform.Scale);

		AABB instanceAABB = { position.x - size.x, position.y - size.y, position.x + size.x, position.y + size.y };

		if (!screenAABB.Overlap(instanceAABB))
			continue;

		RenderQueue2D::RenderInstance instance{};

		glm::vec2 flipMult = glm::vec2(renderer.FlipX ? -1.0f : 1.0f, renderer.FlipY ? -1.0f : 1.0f);

		instance.center = renderer.Center;
		instance.rect = renderer.Rect;
		instance.texture = renderer.TextureHandle;
		instance.zIndex = renderer.RenderLayer;
		instance.transform = transform.ModelMatrix;

		if (scene->SpriteAnimators.HasComponent(entity))
		{
			auto frame = scene->SpriteAnimators.Get(entity).GetCurrentRect();
			instance.rect = frame.Rect;
			instance.transform = glm::translate(instance.transform, glm::vec3(glm::vec2(-scene->SpriteAnimators.Get(entity).GetCurrentRect().Offset) * flipMult, 0.0f));
			if (frame.Rotated)
				instance.transform = glm::rotate(instance.transform, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		}

		instance.transform = glm::rotate(instance.transform, glm::radians(renderer.Rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
		instance.transform = glm::scale(instance.transform, glm::vec3(flipMult, 1.0f));
		instance.color = renderer.SpriteColor;

		if (!renderer.IsGui)
		{
			mRenderQueue.Push(instance);
		}
		else
		{
			mGuiRenderQueue.Push(instance);
		}
	}

	JobManager::Execute([&] { mRenderQueue.Sort(); });
	JobManager::Execute([&] { mGuiRenderQueue.Sort(); });


}

void Renderer2D::Render(Scene* scene, Render::Framebuffer* pOutput)
{
	JobManager::Wait();

	Z_PROFILE_SCOPE("Renderer2D::Render");

	if (mMainPipeline->ShaderDesc.RenderTarget != pOutput)
		mMainPipeline->SetRenderTarget(pOutput);

	float scaledWidth = pOutput->GetSize().x;
	float scaledHeight = pOutput->GetSize().y;

	int scaleFactor = 1;
	int k = 1000;

	for (; scaleFactor < k && scaledWidth / (scaleFactor + 1) >= 320 && scaledHeight / (scaleFactor + 1) >= 180; scaleFactor++) {}

	scaledWidth = scaledWidth / (float)scaleFactor;
	scaledHeight = scaledHeight / (float)scaleFactor;
	int screenWidth = (int)glm::ceil(scaledWidth);
	int screenHeight = (int)glm::ceil(scaledHeight);

	glm::vec2 size = (glm::vec2(scaledWidth, scaledHeight) / 2.0f) * 11.0f;
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

	RenderCamera(&mMainCamera, &mRenderQueue, scene, pOutput);
	RenderCamera(&mGuiCamera, &mGuiRenderQueue, scene, pOutput);

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

void Renderer2D::RenderCamera(Camera2D* camera, RenderQueue2D* renderQueue, Scene* scene, Render::Framebuffer* pOutput)
{
	mSpriteBatch->Begin();

	for (int j = 0; j < renderQueue->instances.size(); j++)
	{
		auto& instance = renderQueue->instances[j];

		mSpriteBatch->DrawSprite(instance.transform, instance.rect, instance.center, instance.color, instance.texture);
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
