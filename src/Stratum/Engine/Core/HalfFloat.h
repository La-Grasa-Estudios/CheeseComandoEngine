#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class half {
public:

	half();
	half(const half& other);
	half(int16_t& h);
	half(float f);

	operator float();
	operator float() const;

	int16_t data;
	
};

class half2 {

public:

	half2();
	half2(const half2& other);
	half2(const half& x, const half& y);
	half2(float x, float y);

	half x, y;

};

class half3 {

public:

	half3();
	half3(const half3& other);
	half3(const half2& xy, const half& z);
	half3(const half& x, const half& y, const half& z);
	half3(float x, float y, float z);

	half x, y, z;

};

class half4 {

public:

	half4();
	half4(const half4& other);
	half4(const half2& xy, const half& z, const half& w);
	half4(const half3& xyz, const half& w);
	half4(const half& x, const half& y, const half& z, const half& w);
	half4(float x, float y, float z, float w);

	half& operator[](int index);

	half x, y, z, w;

};

END_ENGINE