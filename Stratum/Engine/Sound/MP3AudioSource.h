#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"
#include "minimp3/minimp3_ex.h"
#include "SngFile.h"

BEGIN_ENGINE

class MP3AudioSource : public AudioSourceBase
{

public:

	MP3AudioSource(SngFileStream* pStream, ma_engine* pEngine);
	MP3AudioSource(const char* file, ma_engine* pEngine);
	~MP3AudioSource();

	void Rewind() override;
	void UpdateSource() override;

	void Seek(uint32_t samplePosition) override;
	uint32_t Position() override;
	float PositionF() override;

private:

	void Commit(void* ptr, size_t size, size_t frameDivider, size_t nbChannels, size_t sampleRate);
	void UpdateCommit(size_t frameDivider, size_t nbChannels, size_t sampleRate);

	size_t m_CurrentCommitSize = 0;
	char* m_CommitBufferPtr;

	ma_engine* m_Engine;
	RawAudioBuffer* m_AudioBuffer;
	SngFileStream* m_Stream;
	mp3dec_ex_t  m_Mp3DecCtx;
	mp3dec_io_t mp3_io;

	int64_t m_SeekTo = -1;
	int64_t m_Position = 0;
	

};

END_ENGINE