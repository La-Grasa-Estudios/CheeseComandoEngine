#include "Renderer2D.h"

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ConstantBuffer.h"

#include "SpriteBatch.h"

using namespace ENGINE_NAMESPACE;

Renderer2D::Renderer2D()
{
	mPerFrameData = CreateRef<Render::ConstantBuffer>(sizeof(PerFrameData));
	mCmdBuffer = CreateRef<Render::GraphicsCommandBuffer>();
}

void Renderer2D::PreRender(Scene* scene)
{
	
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

		mRenderQueues[renderer.RenderLayer].Push(instance);
	}

}

void Renderer2D::Render(Scene* scene, Render::Framebuffer* pOutput)
{
	mSpriteBatch->Begin();

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < mRenderQueues[i].instances.size(); j++)
		{
			auto& instance = mRenderQueues[i].instances[j];

			mSpriteBatch->DrawSprite(instance.transform, instance.rect, instance.center, instance.texture);
		}
	}

	mCmdBuffer->Begin();

	mCmdBuffer->SetFramebuffer(pOutput);
	mCmdBuffer->SetPipeline(mMainPipeline.get());
	mCmdBuffer->SetConstantBuffer(mPerFrameData.get(), 1);

	mSpriteBatch->End(mCmdBuffer.get());

	mCmdBuffer->End();
}

void Renderer2D::Submit()
{
	mCmdBuffer->Submit();
}
