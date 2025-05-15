#pragma once

#include "Scene.h"
#include "RendererCommon.h"

#include "znmsp.h"

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
	class Framebuffer;
	class GraphicsPipeline;
	class ConstantBuffer;
}

class SpriteBatch;

struct RenderQueue2D
{

	struct RenderInstance
	{
		glm::mat4 transform;
		glm::vec2 center;
		SpriteRendererComponent::SpriteRect rect;
		DescriptorHandle texture;
	};

	std::vector<RenderInstance> instances;

	void Push(const RenderInstance& instance)
	{
		instances.push_back(instance);
	}
	void Clear()
	{
		instances.clear();
	}

};

class Renderer2D
{

public:

	Renderer2D();

	void PreRender(Scene* scene);
	void Render(Scene* scene, Render::Framebuffer* pOutput);
	void Submit();

private:

	struct PerFrameData
	{
		glm::mat4 ProjView;
	};

	Ref<SpriteBatch> mSpriteBatch;
	Ref<Render::GraphicsPipeline> mMainPipeline;

	Ref<Render::GraphicsCommandBuffer> mCmdBuffer;
	Ref<Render::ConstantBuffer> mPerFrameData;
	
	RenderQueue2D mRenderQueues[8];

};

END_ENGINE