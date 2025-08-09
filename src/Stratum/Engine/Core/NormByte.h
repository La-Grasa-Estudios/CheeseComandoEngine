#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class normbyte {
public:

	normbyte();
	normbyte(const normbyte& other);
	normbyte(uint8_t& h);
	normbyte(float f);

	operator float();
	operator float() const;

private:
	uint8_t data;
};

class normbyte4 {

public:

	normbyte4();
	normbyte4(const normbyte4& other);
	//normbyte4(const normbyte2& xy, const normbyte& z, const normbyte& w);
	//normbyte4(const normbyte3& xyz, const normbyte& w);
	normbyte4(const normbyte& x, const normbyte& y, const normbyte& z, const normbyte& w);

	normbyte& operator[](int index);

	normbyte x, y, z, w;

};

END_ENGINE