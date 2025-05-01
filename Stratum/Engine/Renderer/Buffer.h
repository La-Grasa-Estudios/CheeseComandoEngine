#pragma once

#include "znmsp.h"
#include "RendererContext.h"

BEGIN_ENGINE

namespace Render {

	enum class BufferUsage {
		DEFAULT,
		DYNAMIC,
		STAGING,
	};

	enum class BufferType {
		STORAGE,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		INDIRECT_BUFFER,
	};

	enum class BufferComputeType
	{
		UNKNOWN,
		RAW,
		TYPED,
		STRUCTURED,
	};

	struct BufferDescription
	{
		bool AllowComputeResourceUsage = false;
		bool AllowWritesFromMainMemory = false;
		bool AllowReadsToMainMemory = false;
		bool Immutable = false;

		BufferUsage Usage = BufferUsage::DEFAULT;
		BufferType Type = BufferType::STORAGE;
		BufferComputeType ComputeType = BufferComputeType::UNKNOWN;

		nvrhi::Format Format = nvrhi::Format::UNKNOWN;

		void* pSysMem = NULL; // Default data for the buffer
		size_t Size;

		size_t StructuredStride = 4; // Used in compute resources
	};

	class Buffer {

	public:

		Buffer() = default;
		Buffer(const BufferDescription& desc);
		~Buffer();

		void* Map();
		void Unmap();
		bool IsResourceReady();

		BufferDescription BufferDesc;

		nvrhi::BufferHandle Handle;
		nvrhi::SamplerHandle Sampler;

	private:

		bool m_ResourceReady = false;

	};

}

END_ENGINE