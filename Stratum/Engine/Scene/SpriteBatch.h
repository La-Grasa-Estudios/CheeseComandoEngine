#pragma once

#include "znmsp.h"

#include "Entity/Components.h"
#include "SceneResources.h"

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
	class Buffer;
	class VertexBuffer;
}

class SpriteBatch
{

public:

	struct SpriteInstance
	{
		glm::mat4 transform;
		SpriteRendererComponent::SpriteRect rect;
		glm::ivec2 RenderSize;
		glm::vec2 center;
		glm::vec2 offset;
		glm::vec4 color;
		DescriptorHandle texture;
		bool useNearestFilter = false;
	};

	SpriteBatch(SceneResources* pResources);

	void Begin();
	void DrawSprite(const SpriteInstance& instance);
	void End(Render::GraphicsCommandBuffer* pCmdBuffer);

	void SetResources(SceneResources* pRsc);

	struct SpriteRenderable
	{
		glm::mat4 transform;
		glm::vec4 uvs[2];
		glm::vec4 Color;
		DescriptorHandle texture;
		uint32_t flags;
	};

private:

	static inline const uint32_t FLAG_SPRITE_NEAREST = 1 << 0;

	std::vector<SpriteRenderable> mRenderQueue;

	Ref<Render::Buffer> mVbFlatQuad;
	Ref<Render::VertexBuffer> mVbView;

	SceneResources* mResources;

};

END_ENGINE