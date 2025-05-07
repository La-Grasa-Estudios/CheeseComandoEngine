#include "Components.h"

using namespace ENGINE_NAMESPACE;

void TransformComponent::SetWorldPosition(const glm::vec3& position)
{
	glm::vec3 localPosition = glm::inverse(InverseBindMatrix) * glm::vec4(position, 1.0f);
	SetPosition(localPosition);
}

void TransformComponent::SetPosition(const glm::vec3& position)
{
	Position = position;
	IsDirty = true;
}

void TransformComponent::SetScale(const glm::vec3& scale)
{
	Scale = scale;
	IsDirty = true;
}

void TransformComponent::SetRotation(const glm::quat& rotation)
{
	Rotation = rotation;
	IsDirty = true;
}

glm::vec3 TransformComponent::GetWorldSpacePosition() const
{
	return InverseBindMatrix * glm::vec4(Position, 1.0f);
}
