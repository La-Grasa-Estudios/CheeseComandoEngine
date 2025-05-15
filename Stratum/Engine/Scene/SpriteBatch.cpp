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

	float quadVertices[24] = {
		// lower-right triangle
	   -size.x, -size.y, 0.0f, 0.0f,
		 size.x,  size.y, 1.0f, 1.0f,
		 size.x, -size.y, 1.0f, 0.0f
		// upper-left triangle
		-size.x, -size.y, 0.0f, 0.0f, // position, texcoord
		-size.x,  size.y, 0.0f, 1.0f,
		 size.x,  size.y, 1.0f, 1.0f,
	};

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

void SpriteBatch::DrawSprite(const glm::mat4& transform, SpriteRendererComponent::SpriteRect& rect, glm::vec2 center, DescriptorHandle texture)
{

	auto spriteTransform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(center * glm::vec2(rect.size), 0.0f));

	spriteTransform = glm::scale(spriteTransform, glm::vec3(rect.size, 0.0f));

	spriteTransform = transform * spriteTransform;

	SpriteRenderable renderable{};

	renderable.texture = texture;
	renderable.transform = spriteTransform;

	if (texture != -1)
	{
		auto textureHandle = mResources->GetImageHandle(texture);

		if (textureHandle)
		{
			auto size = glm::vec2(textureHandle->GetSize());

			renderable.uvs = { glm::vec2(rect.position) / size, glm::vec2(rect.position + rect.size) / size };
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
		pCmdBuffer->DrawIndexed(4, 0);

	}

}
