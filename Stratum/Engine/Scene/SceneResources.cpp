#include "SceneResources.h"

#include "Renderer/Buffer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/BindlessDescriptorTable.h"

#include "Asset/TextureLoader.h"
#include <Core/Timer.h>
#include <Core/Logger.h>

using namespace ENGINE_NAMESPACE;

SceneResources::~SceneResources()
{
	for (int i = 0; i < mBuffersRefCount.size(); i++) mBuffersRefCount[i] = 0;
	for (int i = 0; i < mMaterialsRefCount.size(); i++) mMaterialsRefCount[i] = 0;
	GarbageCollect();
}

void SceneResources::GarbageCollect()
{
	for (auto i = 0; i < mAllocatedBuffers.size(); i++)
	{
		if (mAllocatedBuffers[i] && mBuffersRefCount[i] <= 0)
		{
			mAllocatedBuffers[i] = false;
			mBuffersRefCount[i] = 0;
			mBufferIndexes[i].Descriptor.Release();
			delete mBufferIndexes[i].pBuffer;
			memset(&mBufferIndexes[i], 0, sizeof(Render::BindlessDescriptorIndex));
		}
	}
	for (auto i = 0; i < mMaterials.size(); i++)
	{
		if (mAllocatedMaterials[i] && mMaterialsRefCount[i] <= 0)
		{
			mAllocatedMaterials[i] = false;
			mMaterialsRefCount[i] = 0;
			delete mMaterials[i].pMaterial;
			mMaterials[i] = {};
		}
	}
}

DescriptorHandle SceneResources::CreateBufferHandle(Render::Buffer* buffer)
{
	DescriptorHandle bufferIndex = AllocateBufferHandle();
	auto bufferDescriptor = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::RawBuffer_SRV(0, buffer->Handle));
	mBufferIndexes[bufferIndex] = { buffer, bufferDescriptor, Render::RendererContext::s_Context->FrameCount };
	return bufferIndex;
}

void SceneResources::ReleaseBufferHandle(DescriptorHandle handle)
{
	memset(&mBufferIndexes[handle], 0, sizeof(Render::BindlessDescriptorIndex));
	_ReleaseBufferHandle(handle);
}

void SceneResources::CreateMeshHandle(Render::Mesh* mesh)
{

	int32_t vertexBufferIndex = AllocateBufferHandle();
	int32_t indexBufferIndex = AllocateBufferHandle();

	auto vertexBufferDescriptor = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::RawBuffer_SRV(0, mesh->GetVertexBuffer()->Handle));
	auto indexBufferDescriptor = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::RawBuffer_SRV(0, mesh->GetIndexBuffer()->Handle));

	mBufferIndexes[vertexBufferIndex] = { mesh->GetVertexBuffer()->GetBuffer(), vertexBufferDescriptor, Render::RendererContext::s_Context->FrameCount};
	mBufferIndexes[indexBufferIndex] = { mesh->GetIndexBuffer()->GetBuffer(), indexBufferDescriptor, Render::RendererContext::s_Context->FrameCount};

	mesh->VertexBufferIndex = vertexBufferIndex;
	mesh->IndexBufferIndex = indexBufferIndex;
}

void SceneResources::ReleaseMeshHandle(Render::Mesh* mesh)
{

	if (mesh->VertexBufferIndex != -1)
	{

		memset(&mBufferIndexes[mesh->VertexBufferIndex], 0, sizeof(Render::BindlessDescriptorIndex));
		memset(&mBufferIndexes[mesh->IndexBufferIndex], 0, sizeof(Render::BindlessDescriptorIndex));

		_ReleaseBufferHandle(mesh->VertexBufferIndex);
		_ReleaseBufferHandle(mesh->IndexBufferIndex);

		mesh->VertexBufferIndex = -1;
		mesh->IndexBufferIndex = -1;

	}

}

std::pair<Render::Material*, DescriptorHandle> SceneResources::CreateMaterial()
{
	DescriptorHandle handle = AllocateMaterialHandle();
	
	mMaterials[handle].pMaterial = new Render::Material();
	mMaterials[handle].NeedsGPUUpdate = true;

	return { mMaterials[handle].pMaterial, handle };
}

void SceneResources::ReleaseMaterial(DescriptorHandle handle)
{
	ReleaseMaterialHandle(handle);
}

DescriptorHandle SceneResources::AllocateMaterialHandle()
{
	int32_t slot = -1;
	for (auto i = 0; i < mAllocatedMaterials.size(); i++)
	{
		if (!mAllocatedMaterials[i])
		{
			slot = i;
			break;
		}
	}
	if (slot == -1)
	{
		size_t oldCapacity = mAllocatedMaterials.size();
		size_t capacity = std::max(64ULL, mAllocatedMaterials.size() * 2);
		mAllocatedMaterials.resize(capacity);
		mMaterialsRefCount.resize(capacity);
		mMaterials.resize(capacity);
		memset(mMaterials.data() + oldCapacity, 0, sizeof(MaterialIndex) * (capacity - oldCapacity));
		memset(mMaterialsRefCount.data() + oldCapacity, 0, sizeof(int32_t) * (capacity - oldCapacity));
		slot = oldCapacity;
	}
	mAllocatedMaterials[slot] = true;
	mMaterialsRefCount[slot] += 1;
	return slot;
}

void SceneResources::ReleaseMaterialHandle(int32_t handle)
{
	if (handle != -1)
	{
		mMaterialsRefCount[handle] -= 1;
	}
}

Render::BindlessDescriptorIndex SceneResources::GetBufferTableIndex(DescriptorHandle handle)
{
	if (mAllocatedBuffers[handle])
	{
		mBufferIndexes[handle].FrameIndex = Render::RendererContext::s_Context->FrameCount;
		return mBufferIndexes[handle].Descriptor;
	}

	return Render::BindlessDescriptorIndex();
}

Render::Material* SceneResources::GetMaterial(DescriptorHandle handle)
{
	if (handle == -1 || handle >= mAllocatedBuffers.size() || !mAllocatedBuffers[handle])
	{
		return nullptr;
	}

	auto mat = mMaterials[handle].pMaterial;

	mMaterials[handle].FrameIndex = Render::RendererContext::s_Context->FrameCount;

	if (mat->Diffuse.GetTexture() && mat->Diffuse.BindlessIndex == -1)
	{
		mat->Diffuse.BindlessIndex = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::Texture_SRV(0, mat->Diffuse.GetTexture()->Handle));
		mMaterials[handle].NeedsGPUUpdate = true;
	}

	if (mat->Normal.GetTexture() && mat->Normal.BindlessIndex == -1)
	{
		mat->Normal.BindlessIndex = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::Texture_SRV(0, mat->Normal.GetTexture()->Handle));
		mMaterials[handle].NeedsGPUUpdate = true;
	}

	if (mat->Metalness.GetTexture() && mat->Metalness.BindlessIndex == -1)
	{
		mat->Metalness.BindlessIndex = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::Texture_SRV(0, mat->Metalness.GetTexture()->Handle));
		mMaterials[handle].NeedsGPUUpdate = true;
	}

	if (mat->Roughness.GetTexture() && mat->Roughness.BindlessIndex == -1)
	{
		mat->Roughness.BindlessIndex = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::Texture_SRV(0, mat->Roughness.GetTexture()->Handle));
		mMaterials[handle].NeedsGPUUpdate = true;
	}

	return mat;
}

Render::Buffer* SceneResources::GetBuffer(DescriptorHandle handle)
{
	if (handle == -1 || !mAllocatedBuffers[handle])
		return nullptr;
	return mBufferIndexes[handle].pBuffer;
}

bool SceneResources::IsMaterialDirty(DescriptorHandle handle)
{
	if (handle == -1)
	{
		return false;
	}

	if (mMaterials[handle].NeedsGPUUpdate)
	{
		mMaterials[handle].NeedsGPUUpdate = false;
		return true;
	}

	return false;
}

DescriptorHandle SceneResources::LoadTextureImage(const std::string& path)
{
	if (auto t = mTextureCache.find(path); t != mTextureCache.end())
	{
		return t->second;
	}

	auto texture = Render::TextureLoader::LoadFileToImage(path, NULL);

#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!texture->IsResourceReady())
	{
		std::this_thread::yield();
#ifdef _DEBUG
		DidWait = true;
#endif
	}

#ifdef _DEBUG
	if (DidWait)
	{
		Z_WARN("CPU Thread stalled for {}ms waiting for resource to be in a ready state! Resource Pointer: {}", timer.GetMillis(), (void*)texture.get());
	}
#endif

	if (texture)
	{
		auto handle = mDescriptorTable->AllocateDescriptor(nvrhi::BindingSetItem::Texture_SRV(0, texture->Handle));
		mTextureCache[path] = handle;
		mTextures[handle] = texture;
		return handle;
	}

	return -1;
}

Render::ImageResource* SceneResources::GetImageHandle(const DescriptorHandle handle)
{
	if (auto a = mTextures.find(handle); a != mTextures.end())
	{
		return a->second.get();
	}
	return nullptr;
}

int32_t SceneResources::AllocateBufferHandle()
{
	int32_t slot = -1;
	for (auto i = mBuffersSearchStart; i < mAllocatedBuffers.size(); i++)
	{
		if (!mAllocatedBuffers[i])
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
	{
		size_t oldCapacity = mAllocatedBuffers.size();
		size_t capacity = std::max(64ULL, mAllocatedBuffers.size() * 2);

		mAllocatedBuffers.resize(capacity);
		mBuffersRefCount.resize(capacity);
		mBufferIndexes.resize(capacity);

		memset(&mBufferIndexes[oldCapacity], 0, sizeof(Render::BindlessDescriptorIndex) * (capacity - oldCapacity));
		memset(&mBuffersRefCount[oldCapacity], 0, sizeof(int32_t) * (capacity - oldCapacity));

		slot = oldCapacity;
	}

	mAllocatedBuffers[slot] = true;
	mBuffersRefCount[slot] += 1;

	return slot;
}

void SceneResources::_ReleaseBufferHandle(int32_t handle)
{
	mBuffersRefCount[handle] -= 1;
}
