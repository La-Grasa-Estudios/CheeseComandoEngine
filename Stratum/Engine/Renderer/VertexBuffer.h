#pragma once

#include "znmsp.h"

#include "Buffer.h"

BEGIN_ENGINE

namespace Render
{

	class VertexBuffer
	{
	public:

		VertexBuffer() = default;
		VertexBuffer(Buffer* pBuffer);
		~VertexBuffer();

		Buffer* GetBuffer();

		nvrhi::BufferHandle Handle;

	private:
		Buffer* m_Buffer;
	};

}

END_ENGINE