#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class normshort {
public:

	normshort();
	normshort(const normshort& other);
	normshort(uint16_t& h);
	normshort(float f);

	operator float();
	operator float() const;

private:
	uint16_t data;
};

class normshort4 {

public:

	normshort4();
	normshort4(const normshort4& other);
	//normshort4(const normshort2& xy, const normshort& z, const normshort& w);
	//normshort4(const normshort3& xyz, const normshort& w);
	normshort4(const normshort& x, const normshort& y, const normshort& z, const normshort& w);

	normshort& operator[](int index);

	normshort x, y, z, w;

};

END_ENGINE