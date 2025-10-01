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
	class GraphicsPipeline;
}

enum class BatchType
{
	UNDEFINED,
	SPRITE,
	TEXT,
	SHAPE,
	IMAGE,
};

class SpriteBatch
{

public:

	struct SpriteInstance
	{
		SpriteRendererComponent::SpriteRect rect;
		glm::ivec2 RenderSize;
		glm::vec2 center;
		glm::vec2 offset;
		glm::vec4 color;
		DescriptorHandle texture;
		bool useNearestFilter = false;
		bool scaleWithRenderSize = true;
		uint32_t UserData;
		Render::GraphicsPipeline* pCustomShader;
		glm::mat4 transform;
	};

	SpriteBatch(SceneResources* pResources);

	void Begin();
	void SetBatch(Render::GraphicsPipeline* pConfig, BatchType batchType);
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

	struct Batch
	{
		BatchType Type;
		Render::GraphicsPipeline* Pipeline;
		uint32_t RenderCount = 0;

		Batch() = default;
		Batch(const Batch& other);
		Batch(Batch&& other);
	};

	void EndBatch();

	static inline const uint32_t FLAG_SPRITE_NEAREST = 1 << 0;

	std::vector<Batch> mBatches;

	size_t mSpriteBufferSize = 0;
	CopySafeResource<Render::Buffer> mSpriteBuffer;

	Ref<Render::Buffer> mVbTextQuad;
	Ref<Render::VertexBuffer> mTextVbView;

	Ref<Render::Buffer> mVbFlatQuad;
	Ref<Render::VertexBuffer> mVbView;

	SceneResources* mResources;

	Batch mCurrentBatch;
	std::vector<SpriteRenderable> mRenderQueue;
	Render::GraphicsPipeline* mShaderBefore;


};

END_ENGINE