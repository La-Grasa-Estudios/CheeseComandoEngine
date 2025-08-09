#pragma once

#include "znmsp.h"

class Timer {

public:

	Timer() {
		m_Start = nanoTime();
	}

	float Get() {
		return (float)(nanoTime() - m_Start) / 1000.0F / 1000.0F / 1000.0F;
	}

	int GetMillis() {
		return (int)((float)(nanoTime() - m_Start) / 1000.0F / 1000.0F);
	}

private:

	long long nanoTime();

	float m_Delta;
	long long m_Start;

};
