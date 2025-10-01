#include "common.as"

mat4 GetMatrix(float p)
{
	p = easeInOutCubic(p);
	float s = mix(0.0f, 1.0f, p);
	mat4 transform = mat4(1.0f);
	transform = scale(transform, vec3(s, s, 1.0f));
	return transform;
}
vec4 GetColor(float p)
{
	p = max((p - 0.6f) / 0.4f, 0.0f);
	return vec4(1.0f, 1.0f, 1.0f, p);
}