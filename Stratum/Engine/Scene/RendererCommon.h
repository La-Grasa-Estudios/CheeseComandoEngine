#pragma once

#include "znmsp.h"

#include <glm/ext.hpp>

BEGIN_ENGINE;

struct ViewPose
{
	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;

	glm::mat4 ProjectionViewMatrix;

	glm::mat4 InverseProjectionMatrix;
	glm::mat4 InverseViewMatrix;
	glm::mat4 InverseProjectionViewMatrix;

	ViewPose() = default;
	ViewPose(const glm::mat4& Proj, const glm::mat4& View);
};

END_ENGINE