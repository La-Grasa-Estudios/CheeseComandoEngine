#include "EngineStats.h"

using namespace ENGINE_NAMESPACE;

#include <mutex>

std::mutex g_TimeLock;

std::vector<ScopedTime> g_Times;

void EngineStats::PushTime(const char* name, float time)
{
	g_TimeLock.lock();
	s_Times.push_back({ name, time });
	g_TimeLock.unlock();
}

void EngineStats::Reset()
{
	g_TimeLock.lock();
	g_Times = s_Times;
	s_Times.clear();
	g_TimeLock.unlock();
}

std::vector<ScopedTime> EngineStats::GetTimes()
{
	return g_Times;
}

ScopedTimer::~ScopedTimer()
{
	if (strncmp(name, "", 1) == 0) return;
	EngineStats::PushTime(this->name, this->timer.Get() * 1000.0f);
}
