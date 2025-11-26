#pragma once

#include "miniaudio/miniaudio.h"
#include "znmsp.h"

#include <Util/HeapArray.h>

#include <future>

BEGIN_ENGINE

struct AudioSourceParams
{
	bool IsPlaying = false;
	bool DoRewind = false;
	bool DoStop = false;
	bool IsReady = false;
	bool ShouldPlay = false;
	bool IsLooping = false;
	bool IsPaused = false;
	bool RemoveOnFinish = false;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	float Pan = 0.5f;
};

enum class WaveformMode
{
	AMPLITUDE_DOMAIN,
	FREQUENCY_DOMAIN
};

class AudioSourceBase
{

public:

	friend class AudioEngine;

	AudioSourceBase();

	virtual void Play();
	virtual void Stop(bool fully = false, bool fadeOut = false, uint32_t fadeOutMillis = 100);
	virtual void Pause();
	virtual void Resume();
	virtual void Rewind() {}

	// Step size is used as the resolution of the internal frequency domain table used for waveform visualization.
	// Domain size is 65536 / stepSize.
	// Higher step sizes mean more detail, but also more CPU usage.
	void EnableWaveform(WaveformMode mode, uint32_t stepSize = 512, float decayFactor = 0.95f);

	virtual void SetVolume(float volume);
	virtual float GetVolume();

	virtual void SetPan(float pan);
	virtual float GetPan();

	virtual void SetPitch(float pitch);
	virtual float GetPitch();

	virtual bool IsPlaying();

	virtual void Seek(uint32_t samplePosition);
	virtual void Seek(float timeInSeconds);
	virtual uint32_t Position();
	virtual float PositionF() { return 0.0f; }

	virtual void UpdateSource() {}
	virtual void AttachToNode(ma_node* pNode, uint32_t index, uint32_t inputIndex);
	void InternalUpdate();

	std::vector<float>& GetFrequencyDomain() { return p_FrequencyDomain; }

	void SetLooping(bool looping);
	
	bool Removed = false;

	std::function<void(int64_t)> OnReadCallback;

	std::string Path;

protected:

	void OnSampleWrite(int16_t* pSamples, uint32_t sampleCount);

	float p_DecayFactor;
	std::vector<float> p_FrequencyDomain;
	std::vector<float> p_SampleBuffer;
	WaveformMode p_WaveformMode = WaveformMode::AMPLITUDE_DOMAIN;

	AudioSourceParams p_Params;

	ma_sound p_Sound;

};

END_ENGINE