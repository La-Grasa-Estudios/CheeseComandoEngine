#include "SpriteBatch.h"

#include "Renderer/ImageResource.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/Buffer.h"
#include "Renderer/VertexBuffer.h"

using namespace ENGINE_NAMESPACE;

SpriteBatch::SpriteBatch(SceneResources* pResources)
{
	this->mResources = pResources;

	const glm::vec2 size = glm::vec2(1.0f);

	float quadVertices[12] = {
		-size.x, size.y, // (0,0) = 0
		-size.x, -size.y, // (0,1) = 1
		 size.x, - size.y, // (1,1) = 2
		-size.x, size.y, // (0,0) = 3
		 size.x, -size.y, // (1,1) = 4
	     size.x, size.y, // (1,0) = 5
	};

	// (0,0) = 0, 0
	// (0,1) = 0, 1
	// (1,0) = 1, 0
	// (1,1) = 1, 1

	Render::BufferDescription quadDesc{};

	quadDesc.Immutable = true;
	quadDesc.Size = sizeof(quadVertices);
	quadDesc.pSysMem = quadVertices;
	quadDesc.Type = Render::BufferType::VERTEX_BUFFER;

	mVbFlatQuad = CreateRef<Render::Buffer>(quadDesc);
	mVbView = CreateRef<Render::VertexBuffer>(mVbFlatQuad.get());

}

void SpriteBatch::Begin()
{
	mRenderQueue.clear();
}

void SpriteBatch::DrawSprite(const glm::mat4& transform, SpriteRendererComponent::SpriteRect& rect, glm::vec2 center, const glm::vec4& color, DescriptorHandle texture)
{

	auto spriteTransform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(center * glm::vec2(rect.size), 0.0f));

	spriteTransform = glm::scale(spriteTransform, glm::vec3(rect.size, 0.0f));

	spriteTransform = transform * spriteTransform;

	SpriteRenderable renderable{};

	renderable.texture = texture;
	renderable.transform = spriteTransform;

	renderable.uvs[0] = glm::vec4(0, 0, 1, 0);
	renderable.uvs[1] = glm::vec4(1, 0, 1, 1);

	renderable.Color = color;

	if (texture != -1)
	{
		auto textureHandle = mResources->GetImageHandle(texture);

		if (textureHandle)
		{
			auto size = glm::vec2(textureHandle->GetSize());

			float x0 = rect.position.x / size.x;
			float y0 = rect.position.y / size.y;
			float x1 = (rect.position.x + rect.size.x) / size.x;
			float y1 = (rect.position.y + rect.size.y) / size.y;

			renderable.uvs[0] = { glm::vec2{ x0, y0 }, glm::vec2{ x0, y1 } };
			renderable.uvs[1] = { glm::vec2{ x1, y0 }, glm::vec2{ x1, y1 } };

		}
		else
		{
			renderable.texture = -1;
		}
	}

	mRenderQueue.push_back(renderable);

}

void SpriteBatch::End(Render::GraphicsCommandBuffer* pCmdBuffer)
{

	if (mRenderQueue.empty())
		return;

	pCmdBuffer->ClearVertexBuffers();
	pCmdBuffer->SetVertexBuffer(mVbView.get(), 0);

	for (auto& renderable : mRenderQueue)
	{

		pCmdBuffer->PushConstants(&renderable, sizeof(SpriteRenderable));
		pCmdBuffer->Draw(6, 0);

	}

}
