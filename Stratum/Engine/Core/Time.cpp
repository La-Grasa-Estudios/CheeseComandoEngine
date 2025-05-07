#include "Time.h"
#include "Logger.h"

#include <chrono>

using namespace ENGINE_NAMESPACE;

long Time::nanoTime()
{
	std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
	auto since_epoch = begin.time_since_epoch();
	return (long)(std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
}

void Time::BeginProfile()
{
	now = nanoTime();
}

void Time::EndProfile()
{
	DeltaTime = (float)(nanoTime() - now) / 1000.0F / 1000.0F / 1000.0F;
	if (DeltaTime * 1000.0f > 100.0f)
	{
		Z_WARN("Frametime got over 100ms! ({}ms), clamping to 100ms", DeltaTime * 1000.0f);
		DeltaTime = 100.0f / 1000.0f;
	}
	GlobalTime += DeltaTime;
}

void Time::BeginRender()
{
}

void Time::EndRender()
{
}

void Time::ClearCPU()
{
	CPUTime = 0.0F;
}

void Time::BeginCPU()
{
	now_cpu = nanoTime();
}

void Time::EndCPU()
{
	CPUTime = (float)(nanoTime() - now_cpu) / 1000.0F / 1000.0F / 1000.0F;
}

void Time::ClearGPU()
{
	GPUTime = 0.0F;
}

void Time::BeginGPU()
{
	now_gpu = nanoTime();
}

void Time::EndGPU()
{
	GPUTime = (float)(nanoTime() - now_gpu) / 1000.0F / 1000.0F / 1000.0F;
}

void Time::ClearUpdate()
{
	UpdateTime = 0.0f;
}

void Time::BeginUpdate()
{
	now_update = nanoTime();
}

void Time::EndUpdate()
{
	UpdateTime += (float)(nanoTime() - now_update) / 1000.0F / 1000.0F / 1000.0F;
}
