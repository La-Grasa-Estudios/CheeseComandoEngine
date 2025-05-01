#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"
#include "SngFile.h"

BEGIN_ENGINE

class SngAudioSource : public AudioSourceBase
{

public:

	SngAudioSource(const char* file, ma_engine* pEngine);
	~SngAudioSource();

	void Play() override;
	void Stop(bool fully, bool fadeOut, uint32_t fadeOutMillis) override;

	void SetVolume(float volume) override;
	//float GetVolume();

	void SetPan(float pan) override;
	//float GetPan();

	void SetPitch(float pitch) override;
	//float GetPitch();

	void Rewind() override;
	void UpdateSource() override;

	bool IsPlaying() override;

	void Seek(uint32_t samplePosition) override;
	uint32_t Position() override;

private:

	Ref<AudioSourceBase> m_AudioSource;
	SngFile* m_pSngFile;
	SngFileMetadata* p_SngFileMetadata;

	ma_engine* pEngine;

	uint32_t LoopStart;
	uint32_t LoopEnd;
	bool DoesLoop;


};

END_ENGINE