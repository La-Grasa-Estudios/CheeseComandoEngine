#pragma once

#include "znmsp.h"

#include "SceneResources.h"
#include "Entity/Components.h"
#include "Util/CopySafeResource.h"

BEGIN_ENGINE

namespace Render
{
	class GraphicsCommandBuffer;
	class CopyCommandBuffer;
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
		uint32_t UserData;
	};

	SpriteBatch(SceneResources* pResources);

	void Begin();
	void DrawSprite(const SpriteInstance& instance);
	void End(Render::CopyCommandBuffer* pCmdBuffer);
	void Render(Render::GraphicsCommandBuffer* pCmdBuffer);

	void SetResources(SceneResources* pRsc);

	struct SpriteRenderable
	{
		glm::mat4 transform;
		glm::vec4 uvs[2];
		glm::vec4 Color;
		DescriptorHandle texture;
		uint32_t flags;
		uint32_t userData;
		uint32_t padding;
	};

private:

	static inline const uint32_t FLAG_SPRITE_NEAREST = 1 << 0;

	std::vector<SpriteRenderable> mRenderQueue;

	size_t mSpriteBufferSize = 0;
	CopySafeResource<Render::Buffer> mSpriteBuffer;
	Ref<Render::Buffer> mVbFlatQuad;
	Ref<Render::VertexBuffer> mVbView;

	SceneResources* mResources;

};

END_ENGINE