#pragma once

#include "znmsp.h"

#include <vector>
#include <mutex>

#include "Renderer/BindlessDescriptorIndex.h"

BEGIN_ENGINE

namespace Render
{
	class Buffer;
	class Mesh;
	class Material;
	class BindlessDescriptorTable;
}

typedef int32_t DescriptorHandle;

class SceneResources
{
public:

	SceneResources() = default;
	SceneResources(Render::BindlessDescriptorTable* pTable)
		: mDescriptorTable(pTable)
	{
	}

	~SceneResources();

	void GarbageCollect();

	DescriptorHandle CreateBufferHandle(Render::Buffer* buffer);
	void ReleaseBufferHandle(DescriptorHandle handle);

	void CreateMeshHandle(Render::Mesh* mesh);
	void ReleaseMeshHandle(Render::Mesh* mesh);

	std::pair<Render::Material*, DescriptorHandle> CreateMaterial();
	void ReleaseMaterial(DescriptorHandle handle);

	Render::BindlessDescriptorIndex GetBufferTableIndex(DescriptorHandle handle);
	Render::Material* GetMaterial(DescriptorHandle handle);
	Render::Buffer* GetBuffer(DescriptorHandle handle);

	bool IsMaterialDirty(DescriptorHandle handle);

private:

	struct BindlessBufferIndex
	{
		Render::Buffer* pBuffer = 0;
		Render::BindlessDescriptorIndex Descriptor;
		uint64_t FrameIndex;
	};

	struct MaterialIndex
	{
		Render::Material* pMaterial;
		uint64_t FrameIndex;
		bool NeedsGPUUpdate;
	};
	
	int32_t AllocateBufferHandle();
	void _ReleaseBufferHandle(int32_t handle);

	int32_t AllocateMaterialHandle();
	void ReleaseMaterialHandle(int32_t handle);

	std::vector<bool> mAllocatedMaterials;
	std::vector<int32_t> mMaterialsRefCount;
	std::vector<MaterialIndex> mMaterials;
	std::vector<bool> mAllocatedBuffers;
	std::vector<int32_t> mBuffersRefCount;
	std::vector<BindlessBufferIndex> mBufferIndexes;
	uint32_t mBuffersSearchStart = 0;

	Render::BindlessDescriptorTable* mDescriptorTable;

};

END_ENGINE