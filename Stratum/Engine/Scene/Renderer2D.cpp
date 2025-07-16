#include "Renderer2D.h"

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ConstantBuffer.h"
#include "Post/PostProcessingStack.h"
#include <Core/EngineStats.h>
#include <Core/Logger.h>
#include <Core/Time.h>
#include <Font/Font.h>
#include <Util/StrUtil.h>
#include <Input/Input.h>

#include "SpriteBatch.h"
#include "TextBatcher.h"

#include "Core/JobManager.h"

#include <format>

using namespace ENGINE_NAMESPACE;

Render::PostProcessingStack* pStack;
Scene* currentRenderScene;

struct DebugLog
{
	LogLevel level;
	std::string message;
	float time;
};

std::binary_semaphore g_DebugSync(1);
std::vector<DebugLog> debugLogs;
bool debugLogAdded = false;

class DebugLogReceiver : LogReceiver
{
public:
	DebugLogReceiver() = default;
protected:
	void Log(std::string_view fmt, LogLevel level) override
	{
		g_DebugSync.acquire();
		DebugLog log;
		log.message = fmt;
		log.time = Time::GlobalTime;
		log.level = level;
		debugLogs.push_back(log);
		g_DebugSync.release();
	}
};

Renderer2D::Renderer2D()
{
	if (!debugLogAdded)
	{
		Logger::s_LogReceivers.push_back(new DebugLogReceiver());
	}

	pStack = new Render::PostProcessingStack();

	mPerFrameData = CreateRef<Render::ConstantBuffer>(sizeof(PerFrameData));
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

	auto& spriteRenderers = scene->SpriteRenderers.GetEntities();
	auto& textRenderers = scene->TextRenderers.GetEntities();

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

	glm::vec2 mouseScaleFactor = scene->VirtualScreenSize / glm::vec2(pOutput->GetSize());
	scene->VirtualMousePosition = Input::GetMousePosition() * mouseScaleFactor;
	scene->VirtualMousePosition -= VirtualScreenSize / 2.0f;
	scene->VirtualMousePosition *= glm::vec2{ 2.0f, -2.0f };

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

	for (auto entity : spriteRenderers)
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
		instance.batch.pCustomShader = renderer.pCustomShader;

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
	for (auto entity : textRenderers)
	{
		if (!scene->Transforms.HasComponent(entity) && scene->TextComponents.HasComponent(entity))
			continue;

		auto& renderer = scene->TextRenderers.Get(entity);

		if (!renderer.Enabled)
			continue;

		RenderQueue2D::RenderInstance instance{};

		instance.kind = RenderQueue2D::RenderInstanceKind::TEXT;
		instance.textEntity = entity;
		instance.zIndex = renderer.RenderLayer;

		if (renderer.IsGui)
			mGuiRenderQueue.Push(instance);
		else
			mRenderQueue.Push(instance);
	}

	// Parallel sorting :D
	JobManager::Execute([&] { mRenderQueue.Sort(); });
	JobManager::Execute([&] { mGuiRenderQueue.Sort(); });

	JobManager::Wait();

	mSpriteBatch->Begin();
	mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::SPRITE);

	TextBatcher textRenderer(mSpriteBatch.get());
	TextBatcherParameters parameters;
	parameters.maxWidth = 800.0f;
	parameters.wrapText = false;
	parameters.fontSize = 64.0f;
	parameters.lineHeight = 1.2f;
	textRenderer.SetParameters(parameters);

	for (int k = 0; k < 2; k++)
	{
		auto& renderQueue = mRenderQueue;
		if (k == 1) renderQueue = mGuiRenderQueue;

		for (int j = 0; j < renderQueue.instances.size(); j++)
		{
			auto& instance = renderQueue.instances[j];

			if (instance.kind == RenderQueue2D::RenderInstanceKind::TEXT)
			{
				auto textEntity = instance.textEntity;
				if (!scene->TextComponents.HasComponent(textEntity))
					continue;

				auto& textComponent = scene->TextComponents.Get(textEntity);
				auto& renderer = scene->TextRenderers.Get(textEntity);
				auto& transform = scene->Transforms.Get(textEntity);

				if (textComponent.Text.empty() || textComponent.Font.empty())
					continue;

				auto font = scene->FontRegistry.GetFont(textComponent.Font);

				if (font && scene->FontRegistry.NeedsUpload(textComponent.Font))
				{
					scene->Resources.CreateFontImage(font);
				}

				parameters.font = font;
				parameters.fontSize = textComponent.FontSize;
				textRenderer.SetParameters(parameters);

				mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::TEXT);

				float offsetX = 0.0f;

				if (renderer.Alignment > 0.0f)
				{
					offsetX = textRenderer.GetStringSize(textComponent.Text).x * renderer.Alignment;
				}

				textRenderer.DrawText(textComponent.Text, glm::vec2(-offsetX, 0.0f), transform.ModelMatrix, renderer.Color, renderer.IsGui);

				continue;
			}
			else
			{
				if (instance.batch.center == glm::vec2(-1.0f, 1.0f))
				{
					mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::TEXT);
					instance.batch.center = glm::vec2(0.0f, 0.0f);
				}
				else
				{
					mSpriteBatch->SetBatch(mMainPipeline.get(), BatchType::SPRITE);
				}
				instance.batch.UserData = k == 0 ? 0 : 1;
				mSpriteBatch->DrawSprite(instance.batch);
			}
		}
	}

	currentRenderScene = scene;

	glm::vec2 scaling = scene->VirtualScreenSize / glm::vec2(pOutput->GetSize());

	auto gd = Render::RendererContext::s_Context->GetGraphicsDeviceProperties();
	auto baseString = std::wstring(L"Stratum Engine {}, {}\nFPS: {} CPU: {:.2f}ms GPU: {:.2f}ms\n{} - {}/{}MB\n");

	mSpriteBatch->SetBatch(nullptr, BatchType::TEXT);

	if (scene->FontRegistry.NeedsUpload("Roboto"))
	{
		scene->Resources.CreateFontImage(scene->FontRegistry.GetFont("Roboto"));
	}

#ifdef DEBUG_RENDERER
	parameters.font = scene->FontRegistry.GetFont("Roboto");
	parameters.fontSize = 64.0f;
	textRenderer.SetParameters(parameters);

	textRenderer.DrawText(Utils::FormatString(baseString, L"" __DATE__, L"" __TIME__,
		(int)(1.0f / Time::UnscaledDeltaTime), Time::UnscaledDeltaTime * 1000.0f, Time::GPURenderTime * 1000.0f,
		Utils::ToWideString(gd.Description).c_str(),
		(int)(gd.UsedVideoMemory / 1024.0f / 1024.0f),
		gd.DedicatedVideoMemory / 1024 / 1024),
		glm::vec2(-VirtualScreenSize.x + 20, VirtualScreenSize.y - 64), glm::identity<glm::mat4>());

	parameters.fontSize = 48;
	parameters.maxWidth = VirtualScreenSize.x*2;
	parameters.wrapText = false;

	textRenderer.SetParameters(parameters);

	float offset = 0;
	g_DebugSync.acquire();
	for (int i = debugLogs.size() - 1; i >= 0; i--)
	{
		auto& log = debugLogs[i];
		if (log.time < Time::GlobalTime - 5.0f)
			continue;
		glm::vec4 color = glm::vec4(1.0f);

		switch (log.level)
		{
		case LogLevel::WARNING:
			color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
			break;
		case LogLevel::LERROR:
			color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			break;
		default:
			break;
		}

		auto str = textRenderer.GetStringSize(Utils::ToWideString(log.message));
		float size_y = str.y * glm::min(str.z, 1.0f);

		textRenderer.DrawText(Utils::ToWideString(log.message), glm::vec2(-VirtualScreenSize.x + 20, -VirtualScreenSize.y + 16 + offset - size_y), glm::identity<glm::mat4>(), color);
		offset += str.y * 1.2f;
	}
	g_DebugSync.release();
#endif
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

	PerFrameData data{};
	data.ProjView[0] = matrices[0];
	data.ProjView[1] = matrices[1];
	data.ScreenSize = pOutput->GetSize();

	mCopyCmdBuffer->UpdateConstantBuffer(mPerFrameData.get(), &data);
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

	for (auto& cbuffer : mCbuffers)
	{
		mCmdBuffer->SetConstantBuffer(cbuffer.pBuffer, cbuffer.Slot);
	}

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

	mCbuffers.clear();
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

Render::Framebuffer* Renderer2D::GetRenderTarget()
{
	return mMainRenderTarget.get();
}

void Renderer2D::RenderCamera(Camera2D* camera, RenderQueue2D* renderQueue, Scene* scene, Render::Framebuffer* pOutput)
{
	// Bindless rendering ftw :D
	
}

void Renderer2D::SetConstantBuffer(Render::ConstantBuffer* pBuffer, uint32_t slot)
{
	if (slot < 2)
		return;

	for (auto& constantBuffer : mCbuffers)
	{
		if (constantBuffer.Slot == slot)
		{
			constantBuffer.pBuffer = pBuffer;
			return;
		}
	}
	mCbuffers.push_back({ pBuffer, slot });
}
