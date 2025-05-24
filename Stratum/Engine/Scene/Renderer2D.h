#pragma once

#include "Scene.h"
#include "RendererCommon.h"

#include "znmsp.h"

#include <algorithm>

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
	class Framebuffer;
	class GraphicsPipeline;
	class ConstantBuffer;
	class TextureSampler;
}

class SpriteBatch;

struct RenderQueue2D
{

	struct RenderInstance
	{
		glm::mat4 transform;
		glm::vec2 center;
		glm::vec4 color;
		uint32_t zIndex;
		SpriteRendererComponent::SpriteRect rect;
		DescriptorHandle texture;

		constexpr bool operator >(const RenderInstance& other) const
		{
			return zIndex > other.zIndex;
		}

		constexpr bool operator <(const RenderInstance& other) const
		{
			return zIndex < other.zIndex;
		}

	};

	std::vector<RenderInstance> instances;

	void Push(const RenderInstance& instance)
	{
		instances.push_back(instance);
	}

	void Sort()
	{
		std::sort(instances.begin(), instances.end(), std::less<RenderInstance>());
	}

	void Clear()
	{
		instances.clear();
	}

};

class Renderer2D
{
	struct PerFrameData
	{
		glm::mat4 ProjView;
	};

	struct Camera2D
	{
		glm::vec2 Position;
		glm::vec2 Zoom = glm::vec2(1.0f);
		float Rotation = 0.0f;
	};

public:

	Renderer2D();

	void PreRender(Scene* scene);
	void Render(Scene* scene, Render::Framebuffer* pOutput);
	void Submit();

	glm::vec2 VirtualScreenSize = {};

	void SetCameraPosition(const glm::vec2& position);
	void SetGuiCameraPosition(const glm::vec2& position);

	void SetCameraZoom(const glm::vec2& zoom);
	void SetGuiCameraZoom(const glm::vec2& zoom);

	void SetCameraRotation(float rotation);
	void SetGuiCameraRotation(float rotation);

private:

	void RenderCamera(Camera2D* camera, RenderQueue2D* renderQueue, Scene* scene, Render::Framebuffer* pOutput);

	Ref<SpriteBatch> mSpriteBatch;
	Ref<Render::GraphicsPipeline> mMainPipeline;

	Ref<Render::GraphicsCommandBuffer> mCmdBuffer;
	Ref<Render::ConstantBuffer> mPerFrameData;

	Ref<Render::TextureSampler> mBilinearSampler;
	
	RenderQueue2D mRenderQueue;
	RenderQueue2D mGuiRenderQueue;

	Camera2D mMainCamera;
	Camera2D mGuiCamera;

};

END_ENGINE