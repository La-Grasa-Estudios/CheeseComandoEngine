#pragma once
#ifndef LADYMATH_HPP
#define LADYMATH_HPP

struct Vec2
{
	float x, y;

	Vec2(float x, float y) : x(x), y(y) {};
	Vec2 operator+(const Vec2& OtherVec) const;
	Vec2 operator-(const Vec2& OtherVec) const;
	Vec2 operator*(const float scalar) const;
	Vec2 operator/(const float scalar) const;
};

#endif //!LADYMATH_HPP