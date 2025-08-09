#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class unormbyte {
public:

	unormbyte();
	unormbyte(const unormbyte& other);
	unormbyte(uint8_t& h);
	unormbyte(float f);

	operator float();
	operator float() const;

private:
	int8_t data;
};

class unormbyte4 {

public:

	unormbyte4();
	unormbyte4(const unormbyte4& other);
	//normbyte4(const normbyte2& xy, const normbyte& z, const normbyte& w);
	//normbyte4(const normbyte3& xyz, const normbyte& w);
	unormbyte4(const unormbyte& x, const unormbyte& y, const unormbyte& z, const unormbyte& w);

	unormbyte& operator[](int index);

	unormbyte x, y, z, w;

};

END_ENGINE