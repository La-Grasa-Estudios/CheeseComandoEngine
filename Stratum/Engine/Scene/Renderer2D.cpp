#include "Renderer2D.h"

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ConstantBuffer.h"
#include "Post/PostProcessingStack.h"
#include <Core/EngineStats.h>
#include <Core/Time.h>
#include <Font/Font.h>
#include <Util/StrUtil.h>

#include "SpriteBatch.h"
#include "TextBatcher.h"

#include "Core/JobManager.h"

#include <format>

using namespace ENGINE_NAMESPACE;

Render::PostProcessingStack* pStack;
Font* font = NULL;
Scene* currentRenderScene;

Renderer2D::Renderer2D()
{
	if (!font)
	{
		font = new Font("Data/fonts/times.ttf");
	}
	else
	{
		font->SetDescriptorHandle(0xFFFFFFFF);
	}

	pStack = new Render::PostProcessingStack();

	mPerFrameData = CreateRef<Render::ConstantBuffer>(sizeof(PerFrameData) * 2);
	mCmdBuffer = CreateRef<Render::GraphicsCommandBuffer>();
	mComputeCmdBuffer = CreateRef<Render::ComputeCommandBuffer>(mCmdBuffer.get());
	mCopyCmdBuffer = CreateRef<Render::CopyCommandBuffer>();

	mMainCamera = {};
	mGuiCamera = {};

	Render::PipelineDescription pipelineDesc{};

	pipelineDesc.ShaderPath = "shaders/2d/2d_sprite.cso";
	pipelineDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(uint32_t)));

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

	samplerDesc.Filter = Render::TextureFilterMode::ANISOTROPIC;
	samplerDesc.AnisoLevel = 16;

	mBilinearSampler = CreateRef<Render::TextureSampler>(samplerDesc);

	samplerDesc.Filter = Render::TextureFilterMode::POINT;

	mNearestSampler = CreateRef<Render::TextureSampler>(samplerDesc);

	samplerDesc.AddressMode = Render::TextureWrapMode::CLAMP;

	mNearestClampSampler = CreateRef<Render::TextureSampler>(samplerDesc);

	samplerDesc.AddressMode = Render::TextureWrapMode::CLAMP;
	samplerDesc.Filter = Render::TextureFilterMode::ANISOTROPIC;
	mBilinearClampSampler = CreateRef<Render::TextureSampler>(samplerDesc);
}

void Renderer2D::PreRender(Scene* scene, Render::Framebuffer* pOutput)
{
	// Did too much thinking about Sparrow animated atlas
	// Finally works! (Thanks DeepSeek)

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

	// Maybe implement proper frustum culling (Or fix this one can be faster since we are checking against an AABB and not 6 planes)
	VirtualScreenSize *= 2.0f;
	AABB screenAABB = { -VirtualScreenSize.x, -VirtualScreenSize.y, VirtualScreenSize.x, VirtualScreenSize.y };
	VirtualScreenSize /= 2.0f;

	if (!mMainRenderTarget || mMainRenderTarget->GetSize() != pOutput->GetSize())
	{
		pStack->Init(pOutput->GetSize());

		Render::ImageDescription imageDesc{};

		imageDesc.AllowComputeResourceUsage = true;
		imageDesc.AllowFramebufferUsage = true;
		imageDesc.Width = pOutput->GetSize().x;
		imageDesc.Height = pOutput->GetSize().y;
		imageDesc.Format = Render::ImageFormat::R11G11B10_FLOAT; // HDR format
		imageDesc.ClearValue = glm::vec4(0.0f);

		mColorBufferRT = CreateRef<Render::ImageResource>(imageDesc);

		Render::FramebufferDesc desc;
		desc.Attachments.push_back({ mColorBufferRT.get() });

		mMainRenderTarget = CreateRef<Render::Framebuffer>(desc);

		mMainPipeline->SetRenderTarget(mMainRenderTarget);
	}

	for (auto entity : entities)
	{

		if (!scene->Transforms.HasComponent(entity))
			continue;

		auto& transform = scene->Transforms.Get(entity);
		auto& renderer = scene->SpriteRenderers.Get(entity);

		if (!renderer.Enabled) continue;

		glm::vec2 position = transform.Position;
		glm::vec2 size = glm::vec2(renderer.Rect.size) * glm::vec2(transform.Scale);

		if (scene->SpriteAnimators.HasComponent(entity))
		{
			auto frame = scene->SpriteAnimators.Get(entity).GetCurrentRect();
			size = frame.Rect.size;
		}

		AABB instanceAABB = { position.x - size.x, position.y - size.y, position.x + size.x, position.y + size.y };

		if (!screenAABB.Overlap(instanceAABB))
			continue;

		RenderQueue2D::RenderInstance instance{};

		glm::vec2 flipMult = glm::vec2(renderer.FlipX ? -1.0f : 1.0f, renderer.FlipY ? -1.0f : 1.0f);

		instance.batch.center = renderer.Center;
		instance.batch.rect = renderer.Rect;
		instance.batch.RenderSize = renderer.Rect.size;
		instance.batch.texture = renderer.TextureHandle;
		instance.batch.transform = transform.ModelMatrix;
		instance.zIndex = renderer.RenderLayer;

		// WE NEED TO ANIMATE IT LOL
		if (scene->SpriteAnimators.HasComponent(entity))
		{
			auto frame = scene->SpriteAnimators.Get(entity).GetCurrentRect();

			auto offset = glm::vec2(frame.Offset);
			auto frameSize = glm::vec2(frame.Rect.size);

			if (frame.Rotated && frame.FrameSize != frame.Rect.size)
			{
				float ox = offset.x;
				offset.x = offset.y;
				offset.y = ox;

				float sx = frameSize.x;
				frameSize.x = frameSize.y;
				frameSize.y = sx;
			}

			instance.batch.rect = frame.Rect;
			instance.batch.RenderSize = frameSize;

			instance.batch.offset = offset;

			// Rotate it, didn't see this in the starling reference sheet until too late, that's why its here
			if (frame.Rotated)
				instance.batch.transform = glm::rotate(instance.batch.transform, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		}

		instance.batch.transform = glm::rotate(instance.batch.transform, glm::radians(renderer.Rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
		instance.batch.transform = glm::scale(instance.batch.transform, glm::vec3(flipMult, 1.0f));
		instance.batch.color = renderer.SpriteColor;
		instance.batch.useNearestFilter = renderer.UseNearestTextureFilter;

		if (!renderer.IsGui)
		{
			mRenderQueue.Push(instance);
		}
		else
		{
			mGuiRenderQueue.Push(instance);
		}
	}

	// Parallel sorting :D
	JobManager::Execute([&] { mRenderQueue.Sort(); });
	JobManager::Execute([&] { mGuiRenderQueue.Sort(); });

	JobManager::Wait();

	mSpriteBatch->Begin();
	mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::SPRITE);

	TextBatcher textRenderer(font, mSpriteBatch.get());
	TextBatcherParameters parameters;
	parameters.maxWidth = 800.0f;
	parameters.wrapText = false;
	parameters.fontSize = 64.0f;
	parameters.lineHeight = 1.2f;
	textRenderer.SetParameters(parameters);

	for (int j = 0; j < mRenderQueue.instances.size(); j++)
	{
		auto& instance = mRenderQueue.instances[j];

		instance.batch.UserData = 0;

		mSpriteBatch->DrawSprite(instance.batch);
	}

	mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::SPRITE);

	for (int j = 0; j < mGuiRenderQueue.instances.size(); j++)
	{
		auto& instance = mGuiRenderQueue.instances[j];

		instance.batch.UserData = 1;

		mSpriteBatch->DrawSprite(instance.batch);
	}

	if (font->GetDescriptorHandle() == 0xFFFFFFFF || currentRenderScene != scene)
	{
		scene->Resources.CreateFontImage(font);
	}

	currentRenderScene = scene;

	glm::vec2 scaling = scene->VirtualScreenSize / glm::vec2(pOutput->GetSize());

	auto gd = Render::RendererContext::s_Context->GetGraphicsDeviceProperties();
	auto baseString = std::wstring(L"Stratum Engine {}, {}\nFPS: {} CPU: {:.2f}ms GPU: {:.2f}ms\n{} - {}/{}MB\n");

	mSpriteBatch->SetBatch(nullptr, BatchType::TEXT);

	textRenderer.DrawText(Utils::FormatString(baseString, L"" __DATE__, L"" __TIME__,
		(int)(1.0f / Time::DeltaTime), Time::DeltaTime * 1000.0f, Time::GPURenderTime * 1000.0f,
		Utils::ToWideString(gd.Description).c_str(),
		(int)(gd.UsedVideoMemory / 1024.0f / 1024.0f),
		gd.DedicatedVideoMemory / 1024 / 1024),
		glm::vec2(-VirtualScreenSize.x + 20, VirtualScreenSize.y - 64), glm::identity<glm::mat4>());

	glm::mat4 matrices[2];

	{
		auto camera = &mMainCamera;
		glm::mat4 proj = glm::ortho(-VirtualScreenSize.x, VirtualScreenSize.x, -VirtualScreenSize.y, VirtualScreenSize.y);
		glm::mat4 view = glm::identity<glm::mat4>();

		view = glm::translate(view, glm::vec3(-camera->Position, 0.0f));
		view = glm::rotate(view, glm::radians(camera->Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		view = glm::scale(view, glm::vec3(camera->Zoom, 1.0f));

		matrices[0] = proj * view;
	}

	{
		auto camera = &mGuiCamera;
		glm::mat4 proj = glm::ortho(-VirtualScreenSize.x, VirtualScreenSize.x, -VirtualScreenSize.y, VirtualScreenSize.y);
		glm::mat4 view = glm::identity<glm::mat4>();

		view = glm::translate(view, glm::vec3(-camera->Position, 0.0f));
		view = glm::rotate(view, glm::radians(camera->Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		view = glm::scale(view, glm::vec3(camera->Zoom, 1.0f));

		matrices[1] = proj * view;
	}

	mCopyCmdBuffer->Begin();

	mCopyCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), matrices);
	mSpriteBatch->End(mCopyCmdBuffer.get());

	mCopyCmdBuffer->End();

	mCopyCmdBuffer->Submit();

}

void Renderer2D::Render(Scene* scene, Render::Framebuffer* pOutput)
{

	Z_PROFILE_SCOPE("Renderer2D::Render");

	Render::Viewport viewport{};

	viewport.width = pOutput->GetSize().x;
	viewport.height = pOutput->GetSize().y;

	mCmdBuffer->Begin();

	mCmdBuffer->RequireFramebufferState(mMainRenderTarget.get(), Render::ResourceState::ShaderResource, Render::ResourceState::RenderTarget);
	mCmdBuffer->RequireFramebufferState(pOutput, Render::ResourceState::Present, Render::ResourceState::RenderTarget);

	mCmdBuffer->ClearBuffer(mMainRenderTarget.get(), 0, glm::vec4(0.0f));

	mCmdBuffer->SetFramebuffer(mMainRenderTarget.get());
	mCmdBuffer->SetViewport(&viewport);

	mCmdBuffer->SetConstantBuffer(mPerFrameData.get(), 1);
	mCmdBuffer->SetTextureSampler(mBilinearSampler.get(), 0);
	mCmdBuffer->SetTextureSampler(mNearestSampler.get(), 1);
	mCmdBuffer->SetBindlessDescriptorTable(scene->BindlessTable);

	mSpriteBatch->Render(mCmdBuffer.get());

	Render::PostProcessingParameters params{};

	params.gCommandBuffer = mCmdBuffer.get();
	params.cCommandBuffer = mComputeCmdBuffer.get();
	params.pBilinearTextureSampler = mBilinearClampSampler.get();
	params.pNearestTextureSampler = mNearestClampSampler.get();
	params.OutputResolution = pOutput->GetSize();
	params.Resolution = pOutput->GetSize();
	params.pOutputFramebuffer = pOutput;
	params.pColorSampler = mColorBufferRT.get();

	mCmdBuffer->SetBindlessDescriptorTable(NULL);
	mCmdBuffer->SetFramebuffer(NULL);

	mCmdBuffer->RequireFramebufferState(mMainRenderTarget.get(), Render::ResourceState::RenderTarget, Render::ResourceState::ShaderResource);

	pStack->Render(params);

	mCmdBuffer->End();
}

void Renderer2D::Submit()
{
	// Separated so i can easily implement multithreaded submission when i get a cpu bottleneck
	// Cpu rendering takes about 1ms so i don't care lol
	mCopyCmdBuffer->TriggerWaitOnExecutionQueue(Render::CommandQueue::Graphics);
	mCmdBuffer->Submit();
}

void Renderer2D::UpdateScreenSize(const glm::ivec2& size)
{
	// Stole this from an older project of mine because i forgot how to implement proper UI scaling lmao
	float scaledWidth = size.x;
	float scaledHeight = size.y;

	int scaleFactor = 1;
	int k = 1000;

	for (; scaleFactor < k && scaledWidth / (scaleFactor + 1) >= 320 && scaledHeight / (scaleFactor + 1) >= 180; scaleFactor++) {}

	scaledWidth = scaledWidth / (float)scaleFactor;
	scaledHeight = scaledHeight / (float)scaleFactor;
	int screenWidth = (int)glm::ceil(scaledWidth);
	int screenHeight = (int)glm::ceil(scaledHeight);

	glm::vec2 ssize = (glm::vec2(scaledWidth, scaledHeight) / 2.0f) * 11.0f;
	VirtualScreenSize = ssize;
}

// This works for the game i'm making now as of 17/05/2025
// But can easily be expanded so i can use the ECS to specify camera parameters

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
	// Bindless rendering ftw :D
	
}
