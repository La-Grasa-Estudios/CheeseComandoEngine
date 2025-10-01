#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"

BEGIN_ENGINE

class SameAudioSource : public AudioSourceBase
{

public:

	SameAudioSource(ma_engine* pEngine);
	~SameAudioSource();

	void Rewind() override;
	void UpdateSource() override;

	void Seek(uint32_t samplePosition) override;
	uint32_t Position() override;
	float PositionF() override;

private:
	ma_engine* pEngine;
	uint32_t m_Position = 0;
	RawAudioBuffer* m_AudioBuffer = nullptr;
	char audio_ctx[48];
};

END_ENGINE