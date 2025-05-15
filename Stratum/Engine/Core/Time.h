#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class Time {

	inline static long now = 0L;
	inline static long now_cpu = 0L;
	inline static long now_gpu = 0L;
	inline static long now_update = 0L;

public:

	inline static float DeltaTime = 0.0F;
	inline static float GlobalTime = 0.0F; // Aproximate Time Since Startup
	inline static float FixedDeltaTime = 1.0F / 50.0F;

	inline static float CPUTime = 0.0F;
	inline static std::atomic<float> GPUTime = 0.0F;
	inline static float UpdateTime = 0.0F;

	static long nanoTime();

	static void BeginProfile();
	static void EndProfile();

	static void BeginRender();
	static void EndRender();

	static void ClearCPU();
	static void BeginCPU();
	static void EndCPU();

	static void ClearGPU();
	static void PushGPU(float time);

	static void ClearUpdate();
	static void BeginUpdate();
	static void EndUpdate();

};

END_ENGINE
