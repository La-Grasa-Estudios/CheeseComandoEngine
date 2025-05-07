#include "RendererCommon.h"

using namespace ENGINE_NAMESPACE;

ViewPose::ViewPose(const glm::mat4& Proj, const glm::mat4& View)
{
	ProjectionMatrix = Proj;
	ViewMatrix = View;

	ProjectionViewMatrix = Proj * View;

	InverseProjectionMatrix = glm::inverse(Proj);
	InverseViewMatrix = glm::inverse(View);
	InverseProjectionViewMatrix = glm::inverse(ProjectionViewMatrix);
}
