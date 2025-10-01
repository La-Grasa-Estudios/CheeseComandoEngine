#include "RendererCommon.h"

#include <Scene/Scene.h>

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

glm::mat4 RenderUtil::GetProjectionMatrix(ECS::edict_t entity, Scene* scene)
{
	auto& camera = scene->Cameras.Get(entity);

	if (camera.Orthographic)
	{
		if (camera.RendersToGui)
		{
			camera.OrthographicSize = scene->VirtualScreenSize;
		}

		glm::vec2 size = camera.OrthographicSize;
		size *= camera.OrthographicZoom;
		return glm::ortho(-size.x, size.x, -size.y, size.y);
	}

	return glm::mat4();
}

glm::mat4 RenderUtil::GetViewMatrix(ECS::edict_t entity, Scene* scene)
{
	auto& camera = scene->Cameras.Get(entity);
	auto& transform = scene->Transforms.Get(entity);

	glm::mat4 view = glm::identity<glm::mat4>();

	view = glm::translate(view, -transform.Position);
	view *= glm::mat4_cast(transform.Rotation);

	return view;
}
