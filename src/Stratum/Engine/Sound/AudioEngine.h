#pragma once

#include "AudioSourceBase.h"
#include "Core/Ref.h"

#include <thread>

#define MAX_SOURCES 4096

BEGIN_ENGINE

class AudioEngine
{
public:

	AudioEngine();

	void Init();
	void Shutdown();

	void Update();

	void AddSource(Ref<AudioSourceBase> source);
	
	void StopAll();

	ma_engine* GetEngine();

	inline static AudioEngine* s_Instance = NULL;

private:

	std::thread* pAudioThread;
	std::atomic_bool m_AudioThreadRun;

	ma_engine m_Engine;
	Ref<AudioSourceBase> m_Sources[MAX_SOURCES];
};

END_ENGINE