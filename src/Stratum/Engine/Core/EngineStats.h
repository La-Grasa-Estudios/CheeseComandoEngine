#pragma once

#include "znmsp.h"

#include "Core/Timer.h"

#include <vector>
#include <atomic>
#include <atomic>

#define PASTER(x,y) x ## _ ## y
#define EVALUATOR(x,y)  PASTER(x,y)
#define Z_PROFILE_SCOPE(name) ENGINE_NAMESPACE::ScopedTimer EVALUATOR(Timer, __LINE__) { name };

BEGIN_ENGINE

struct ScopedTime
{
	const char* name;
	float time;
};

struct ScopedTimer
{
	const char* name;
	Timer timer;
	~ScopedTimer();
};

class EngineStats
{

public:

	static void PushTime(const char* name, float time);
	static void PushStat(const char* name);
	static void Reset();

	static void IncStat(const char* name);

	static std::vector<ScopedTime> GetTimes();

private:

	static inline std::vector<ScopedTime> s_Times;

};

END_ENGINE