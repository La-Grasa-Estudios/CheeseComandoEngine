#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"
#include "brstm_codec.h"

BEGIN_ENGINE

class BRSTMAudioSource : public AudioSourceBase
{

public:

	BRSTMAudioSource(BRSTM::BRSTMFile* pFile, ma_engine* pEngine);
	~BRSTMAudioSource();

	void Rewind() override;
	void UpdateSource() override;

private:

	void Commit(void* ptr, size_t size, size_t frameDivider, size_t nbChannels, size_t sampleRate);
	void UpdateCommit(size_t frameDivider, size_t nbChannels, size_t sampleRate);

	size_t m_CurrentCommitSize = 0;
	char* m_CommitBufferPtr;

	struct loop_params_t
	{
		uint32_t WaveSize = 0;
		uint32_t WaveOffset = 0;
		bool Valid = false;
	};

	bool TryProcessChunk();

	BRSTM::BRSTMStream* m_Stream;
	BRSTM::BRSTMChunk* m_NextChunk;
	uint32_t m_LoopChunk = 0;
	uint32_t m_LoopSamples = 0;
	uint32_t m_LoopCount = 0;
	loop_params_t m_LoopParams;
	ma_engine* m_Engine;
	RawAudioBuffer* m_AudioBuffer;

};

END_ENGINE