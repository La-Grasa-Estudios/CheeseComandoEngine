#include "BRSTMAudioSource.h"

using namespace ENGINE_NAMESPACE;

BRSTMAudioSource::BRSTMAudioSource(BRSTM::BRSTMFile* pFile, ma_engine* pEngine)
{
	m_Stream = new BRSTM::BRSTMStream(pFile);
	m_Engine = pEngine;
    m_NextChunk = NULL;
    m_AudioBuffer = NULL;
}

BRSTMAudioSource::~BRSTMAudioSource()
{
	delete m_Stream;
	delete m_AudioBuffer;
    if (p_IsSoundInitialized) ma_sound_uninit(&p_Sound);
    if (m_CommitBufferPtr) delete[] m_CommitBufferPtr;
}

void BRSTMAudioSource::Rewind()
{
	Stop();
    if (m_AudioBuffer) m_AudioBuffer->Clear();
	m_Stream->rewind();
    if (m_NextChunk) delete m_NextChunk;
    m_NextChunk = NULL;
    m_LoopCount = 0;
	//Play();
}

void BRSTMAudioSource::UpdateSource()
{
    TryProcessChunk();
}

void BRSTMAudioSource::Commit(void* ptr, size_t size, size_t frameDivider, size_t nbChannels, size_t sampleRate)
{
    size_t maxByteSize = sampleRate * frameDivider * nbChannels;

    size_t offset = 0;
    size_t breakcount = 1;
    size_t osize = size;

    while (m_CurrentCommitSize + size > maxByteSize)
    {
        size_t rem = maxByteSize - m_CurrentCommitSize;
        if (rem > size)
        {
            rem = size;
        }
        memcpy(m_CommitBufferPtr + m_CurrentCommitSize, (char*)ptr + offset, rem);
        offset += rem;
        size -= rem;
        m_CurrentCommitSize += rem;
        UpdateCommit(frameDivider, nbChannels, sampleRate);
        breakcount++;
    }

    if (size > 0)
    {
        memcpy(m_CommitBufferPtr + m_CurrentCommitSize, (char*)ptr + offset, size);
        m_CurrentCommitSize += size;

        UpdateCommit(frameDivider, nbChannels, sampleRate);
    }
    
}

void BRSTMAudioSource::UpdateCommit(size_t frameDivider, size_t nbChannels, size_t sampleRate)
{
    size_t maxByteSize = sampleRate * frameDivider * nbChannels;
    if (m_CurrentCommitSize >= maxByteSize)
    {
        m_AudioBuffer->Queue(m_CommitBufferPtr, m_CurrentCommitSize / frameDivider);
        m_CurrentCommitSize = 0;
    }
}

bool BRSTMAudioSource::TryProcessChunk()
{

    if (p_ShouldReset)
    {
        p_ShouldReset = false;
        Rewind();
    }

    if (!p_ShouldBePlaying) return false;

    if (!m_NextChunk) m_NextChunk = m_Stream->get_next_chunk();

    if (!m_NextChunk) return false;

    int samples = m_Stream->positionInSamples - m_NextChunk->waveHeader.sampleRate;

    uint32_t waveSize = m_NextChunk->waveHeader.subchunk2Size;
    uint32_t waveOffset = 0;

    if (!m_AudioBuffer)
    {
        m_AudioBuffer = new Stratum::RawAudioBuffer(m_NextChunk->waveHeader.sampleRate, m_NextChunk->waveHeader.numChannels, m_NextChunk->waveHeader.bitsPerSample == 16 ? ma_format_s16 : ma_format_u8, m_NextChunk->waveHeader.sampleRate * 3);
        m_CommitBufferPtr = new char[m_NextChunk->waveHeader.sampleRate * (m_NextChunk->waveHeader.bitsPerSample / 8) * m_NextChunk->waveHeader.numChannels];
        
        ma_sound_init_from_data_source(m_Engine, m_AudioBuffer->GetDataSource(), 0, NULL, &p_Sound);

        ma_node_attach_output_bus(&p_Sound, 0, m_Engine, 0);

        p_IsSoundInitialized = true;
    }

    if (!m_AudioBuffer->CanWrite((waveSize / (m_NextChunk->waveHeader.bitsPerSample / 8))))
    {
        return false;
    }

    if (m_Stream->file->header.loop) {

        uint32_t loopStart = m_Stream->file->header.loopStart;
        uint32_t loopEnd = m_Stream->file->header.loopSize;
        uint32_t loopChunk = loopEnd - (loopEnd % m_NextChunk->waveHeader.sampleRate);

        if (samples + m_NextChunk->get_chunk_samples() >= loopStart && m_LoopChunk == 0) {

            m_LoopChunk = m_Stream->filePosition - m_NextChunk->get_chunk_size();
            m_LoopSamples = samples;
            std::cout << "Loop pos " << m_LoopChunk << " Samples " << m_LoopSamples << "\n";

        }



        if (m_LoopParams.Valid && m_LoopChunk != 0) {

            m_LoopParams.Valid = false;

            int32_t extraSamples = loopStart - m_LoopSamples;

            if (extraSamples > 0) {

                int32_t extraBytes = extraSamples * (m_NextChunk->waveHeader.bitsPerSample / 8) * m_NextChunk->waveHeader.numChannels;
                waveOffset = extraBytes;
                waveSize -= extraBytes;
            }

            //std::cout << "Loop start " << loopStart << " loop at file " << m_LoopSamples << " Samples " << extraSamples << "\n";

        }

        if ((uint32_t)samples >= loopChunk) {

            int32_t extraSamples = m_NextChunk->get_chunk_samples() - (loopEnd % m_NextChunk->waveHeader.sampleRate);
            if (extraSamples > 0) {
                int32_t extraBytes = extraSamples * (m_NextChunk->waveHeader.bitsPerSample / 8) * m_NextChunk->waveHeader.numChannels;
                waveSize -= extraBytes;
            }

            waveOffset = 0;

            m_Stream->rewind();

            m_Stream->filePosition = m_LoopChunk;
            m_Stream->positionInSamples = m_LoopSamples;

            m_LoopParams.Valid = true;

            m_LoopCount++;

        }

    }

    ENGINE_NAMESPACE::BRSTM::WAVEData waveData = m_NextChunk->get_samples();
    char* ptr = waveData.data;

    Commit(ptr + waveOffset, waveSize, m_NextChunk->waveHeader.bitsPerSample / 8, m_NextChunk->waveHeader.numChannels, m_NextChunk->waveHeader.sampleRate);

    delete[] ptr;
    delete m_NextChunk;
    m_NextChunk = 0;

    if (!ma_sound_is_playing(&p_Sound) || ma_sound_at_end(&p_Sound))
    {
        ma_sound_start(&p_Sound);
    }

	return true;
}
