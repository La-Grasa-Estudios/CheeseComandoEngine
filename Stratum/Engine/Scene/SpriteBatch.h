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

	SpriteBatch(SceneResources* pResources);

	void Begin();
	void DrawSprite(const glm::mat4& transform, SpriteRendererComponent::SpriteRect& rect, glm::vec2 center, const glm::vec4& color, DescriptorHandle texture);
	void End(Render::GraphicsCommandBuffer* pCmdBuffer);

	void SetResources(SceneResources* pRsc);

	struct SpriteRenderable
	{
		glm::mat4 transform;
		glm::vec4 uvs[2];
		glm::vec4 Color;
		DescriptorHandle texture;
	};

private:

	std::vector<SpriteRenderable> mRenderQueue;

	Ref<Render::Buffer> mVbFlatQuad;
	Ref<Render::VertexBuffer> mVbView;

	SceneResources* mResources;

};

END_ENGINE