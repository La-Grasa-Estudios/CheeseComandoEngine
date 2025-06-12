#include "PostProcessingCommon.h"

using namespace ENGINE_NAMESPACE;

void Render::PostProcessingParameters::DispatchScreenSize(ComputeCommandBuffer* cmd, uint32_t cellSize, glm::ivec2 size) const
{
	uint32_t x = glm::ceil(static_cast<float>(size.x) / cellSize);
	uint32_t y = glm::ceil(static_cast<float>(size.y) / cellSize);

	cmd->Dispatch(x, y, 1);
}
