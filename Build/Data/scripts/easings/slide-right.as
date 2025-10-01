#include "common.as"

mat4 GetMatrix(float p)
{
	p = easeInOutCubic(p);
	float s = mix(1.0f, 0.0f, p);
	mat4 transform = mat4(1.0f);
	transform = translate(transform, vec3(s * 4000.0f, 0.0f, 1.0f));
	return transform;
}
vec4 GetColor(float p)
{
	return vec4(1.0f, 1.0f, 1.0f, 1.0f);
}