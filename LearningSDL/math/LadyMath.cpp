#include "LadyMath.hpp"

Vec2 Vec2::operator+(const Vec2& OtherVec) const
{
	return Vec2(x + OtherVec.x, y + OtherVec.y);
}

Vec2 Vec2::operator-(const Vec2&  OtherVec) const
{
	return Vec2(x - OtherVec.x, y - OtherVec.y);
}

Vec2 Vec2::operator*(const float scalar) const
{
	return Vec2(x * scalar, y * scalar);
}

Vec2 Vec2::operator/(const float scalar) const
{
	return Vec2(x / scalar, y / scalar);
}