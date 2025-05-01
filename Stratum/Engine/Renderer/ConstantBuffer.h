#pragma once

#include "znmsp.h"
#include "RendererContext.h"

#include <glm/glm.hpp>

BEGIN_ENGINE

namespace Render {


	class ConstantBuffer {

	public:

		ConstantBuffer(size_t size, void* defaultData = NULL);
		~ConstantBuffer();

		bool IsResourceReady();

		nvrhi::BufferHandle Handle;
		size_t Size;

	private:

		bool mIsReady = false;

	};

}

END_ENGINE