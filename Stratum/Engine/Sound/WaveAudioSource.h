#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"
#include "SngFile.h"

BEGIN_ENGINE

class WaveAudioSource : public AudioSourceBase
{

public:

	WaveAudioSource(SngFileStream* pStream, ma_engine* pEngine);
	WaveAudioSource(const char* file, ma_engine* pEngine);
	~WaveAudioSource();

	void Rewind() override;
	void UpdateSource() override;

	void Seek(uint32_t samplePosition) override;
	uint32_t Position() override;

private:

	ma_engine* m_Engine;
	RawAudioBuffer* m_AudioBuffer;
	SngFileStream* m_Stream;

	int64_t m_SeekTo = -1;
	int64_t m_Position = 0;

	char waveHeaderBuffer[44] = {};


};

END_ENGINE