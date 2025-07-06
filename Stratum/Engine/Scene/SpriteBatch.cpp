#include "SpriteBatch.h"

#include "Renderer/ImageResource.h"
#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/CopyCommandBuffer.h"
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

	// It took soo much time to get this right in HLSL land
	// This sheet here helped me a lot to understand which corners are what uvs
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

void SpriteBatch::DrawSprite(const SpriteInstance& instance)
{
	auto offset = instance.offset;

	glm::vec2 scaleFactor = glm::vec2(instance.rect.size) / glm::vec2(instance.RenderSize);

	if (!instance.scaleWithRenderSize)
	{
		scaleFactor = glm::vec2(1.0f, 1.0f);
	}

	glm::vec2 pivotOffset = glm::vec2(instance.offset) / glm::vec2(instance.RenderSize);

	auto spriteTransform = glm::identity<glm::mat4>();

	spriteTransform = glm::scale(spriteTransform, glm::vec3(scaleFactor, 1.0f));
	spriteTransform = glm::translate(spriteTransform, glm::vec3(-pivotOffset, 0.0f));
	spriteTransform = glm::scale(spriteTransform, glm::vec3(glm::vec2(instance.RenderSize) * scaleFactor, 1.0f));
	spriteTransform = glm::translate(spriteTransform, glm::vec3(instance.center, 0.0f));

	spriteTransform = instance.transform * spriteTransform;

	SpriteRenderable renderable{};

	renderable.texture = instance.texture;
	renderable.transform = spriteTransform;

	renderable.uvs[0] = glm::vec4(0, 0, 1, 0);
	renderable.uvs[1] = glm::vec4(1, 0, 1, 1);

	renderable.Color = instance.color;
	renderable.userData = instance.UserData;

	if (instance.texture != -1)
	{
		auto textureHandle = mResources->GetImageHandle(instance.texture);

		if (textureHandle)
		{
			auto size = glm::vec2(textureHandle->GetSize());

			float x0 = instance.rect.position.x / size.x;
			float y0 = instance.rect.position.y / size.y;
			float x1 = (instance.rect.position.x + instance.rect.size.x) / size.x;
			float y1 = (instance.rect.position.y + instance.rect.size.y) / size.y;

			// Looks awful but i need it bc of 16 byte alignment
			renderable.uvs[0] = { glm::vec2{ x0, y0 }, glm::vec2{ x0, y1 } };
			renderable.uvs[1] = { glm::vec2{ x1, y0 }, glm::vec2{ x1, y1 } };

		}
		else
		{
			renderable.texture = -1; // Shader automatically colors it white
		}
	}

	// Is bit manipulation slow on modern gpus?
	if (instance.useNearestFilter)
	{
		renderable.flags |= FLAG_SPRITE_NEAREST;
	}

	mRenderQueue.push_back(renderable);

}

void SpriteBatch::Render(Render::GraphicsCommandBuffer* pCmdBuffer)
{
	if (mRenderQueue.empty()) // No need to render if there isn't anything in the queue
		return;

	pCmdBuffer->ClearVertexBuffers();
	pCmdBuffer->SetVertexBuffer(mVbView.get(), 0);
	pCmdBuffer->SetBufferResource(mSpriteBuffer.GetPointer(), 10);

	// Now all stuff is rendered in one single drawcall
	// NonUniformResourceIndex is required to avoid artifacts on some gpus
	pCmdBuffer->DrawInstanced(6, 0, mRenderQueue.size(), 0);
}

void SpriteBatch::End(Render::CopyCommandBuffer* pCmdBuffer)
{
	if (mRenderQueue.empty()) // No need to render if there isn't anything in the queue
		return;

	if (mSpriteBufferSize < mRenderQueue.size())
	{
		mSpriteBufferSize = mRenderQueue.size();
		Render::BufferDescription desc{};
		desc.Size = mSpriteBufferSize * sizeof(SpriteRenderable);
		desc.Type = Render::BufferType::STORAGE;
		desc.ComputeType = Render::BufferComputeType::STRUCTURED;
		desc.StructuredStride = sizeof(SpriteRenderable);
		desc.Immutable = false;
		mSpriteBuffer = CopySafeResource<Render::Buffer>(desc);
	}

	pCmdBuffer->GetNativeCommandList()->writeBuffer(mSpriteBuffer->Handle, mRenderQueue.data(), mRenderQueue.size() * sizeof(SpriteRenderable), 0);
}

void SpriteBatch::SetResources(SceneResources* pRsc)
{
	mResources = pRsc; // This used to crash when changing scenes, my bad since i didn't test scene changing until 3 years after i started developing this branch of the engine, now it works and is very fast (<0.5ms) :D
}
