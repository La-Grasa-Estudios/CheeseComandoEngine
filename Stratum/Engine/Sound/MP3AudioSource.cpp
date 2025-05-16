#define MINIMP3_IMPLEMENTATION
#include "MP3AudioSource.h"

#include "Core/Logger.h"
#include "VFS/ZVFS.h"

using namespace ENGINE_NAMESPACE;

size_t mp3d_read_callback(void* buff, size_t size, void* stream)
{
    SngFileStream* st = reinterpret_cast<SngFileStream*>(stream);
    return st->read(buff, size);
}
int mp3d_seek_callback(uint64_t seekpos, void* stream)
{
    SngFileStream* st = reinterpret_cast<SngFileStream*>(stream);
    st->seekg(seekpos);
    return 0;
}

MP3AudioSource::MP3AudioSource(SngFileStream* pStream, ma_engine* pEngine)
{
    m_AudioBuffer = NULL;
    m_Engine = pEngine;
    m_Stream = pStream;

    mp3_io.read = &mp3d_read_callback;
    mp3_io.read_data = m_Stream;
    mp3_io.seek = &mp3d_seek_callback;
    mp3_io.seek_data = m_Stream;

    mp3dec_ex_open_cb(&m_Mp3DecCtx, &mp3_io, MP3D_SEEK_TO_SAMPLE);
}

MP3AudioSource::MP3AudioSource(const char* file, ma_engine* pEngine)
{

    m_AudioBuffer = NULL;
    m_Engine = pEngine;

    m_Stream = new SngFileStream(ZVFS::GetFileStream(file));

    if (!m_Stream->is_open())
    {
        return;
    }

    mp3_io.read = &mp3d_read_callback;
    mp3_io.read_data = m_Stream;
    mp3_io.seek = &mp3d_seek_callback;
    mp3_io.seek_data = m_Stream;

    mp3dec_ex_open_cb(&m_Mp3DecCtx, &mp3_io, MP3D_SEEK_TO_SAMPLE);
    
}

MP3AudioSource::~MP3AudioSource()
{
    delete m_AudioBuffer;
    if (p_Params.IsReady) ma_sound_uninit(&p_Sound);
    if (m_Stream->is_self_contained()) delete m_Stream;
}

void MP3AudioSource::Rewind()
{
    p_Params.DoRewind = true;
}

void MP3AudioSource::UpdateSource()
{

    if (p_Params.DoStop && p_Params.IsReady)
    {
        p_Params.DoStop = false;
        p_Params.ShouldPlay = false;
        p_Params.IsPlaying = false;
        ma_sound_stop(&p_Sound);
        return;
    }

    if (!p_Params.ShouldPlay)
    {
        return;
    }

    if (!m_AudioBuffer)
    {
        m_AudioBuffer = new Stratum::RawAudioBuffer(m_Mp3DecCtx.info.hz, m_Mp3DecCtx.info.channels, ma_format_s16, m_Mp3DecCtx.info.hz / 4);
        //m_CommitBufferPtr = new char[m_Mp3DecCtx.info.hz * sizeof(short) * m_Mp3DecCtx.info.channels * 8];

        ma_sound_init_from_data_source(m_Engine, m_AudioBuffer->GetDataSource(), 0, NULL, &p_Sound);

        ma_node_attach_output_bus(&p_Sound, 0, m_Engine, 0);

        p_Params.IsReady = true;
    }

    if (p_Params.IsReady)
    {

        if (p_Params.DoRewind)
        {
            p_Params.DoRewind = false;
            if (m_AudioBuffer) m_AudioBuffer->Clear();
            mp3dec_ex_seek(&m_Mp3DecCtx, 0);
        }

        ma_sound_set_volume(&p_Sound, p_Params.Volume);
        ma_sound_set_pitch(&p_Sound, p_Params.Pitch);
        ma_sound_set_pan(&p_Sound, p_Params.Pan);
    }

    //Z_INFO("Ay peter si eres bello chamo uwu <3") // Jorge 08-11-2024
    
    if (m_SeekTo != -1)
    {
        m_AudioBuffer->Clear();
        mp3dec_ex_seek(&m_Mp3DecCtx, m_SeekTo * m_Mp3DecCtx.info.channels);
        m_Position = m_SeekTo;
        m_SeekTo = -1;
    }

    const size_t alignedSize = 4;
    size_t samples = 1;
    bool fl = true;

    mp3d_sample_t buffer[alignedSize];// = (mp3d_sample_t*)malloc(alignedSize * 2 * sizeof(mp3d_sample_t));

    while (m_AudioBuffer->CanWrite(alignedSize))
    {
        
        if (fl)
        {
            fl = false;
            samples = 0;
        }

        size_t readed = mp3dec_ex_read(&m_Mp3DecCtx, buffer, alignedSize);
        
        if (readed != alignedSize) /* normal eof or error condition */
        {
            
            if (m_Mp3DecCtx.last_error)
            {
                break;
            }
            if (readed == 0)
            {
                break;
            }
        }

        m_Position += m_Mp3DecCtx.info.channels;

        if (OnReadCallback)
            OnReadCallback(m_Position);

        if (m_SeekTo != -1)
        {
            mp3dec_ex_seek(&m_Mp3DecCtx, m_SeekTo * m_Mp3DecCtx.info.channels);
            m_Position = m_SeekTo * m_Mp3DecCtx.info.channels;
            m_SeekTo = -1;
        }

        m_AudioBuffer->Queue(buffer, readed);
        samples += readed;
    }

    if (!samples)
    {
        p_Params.ShouldPlay = false;
        p_Params.IsPlaying = false;
    }

    if (!ma_sound_is_playing(&p_Sound) || ma_sound_at_end(&p_Sound))
    {
        ma_sound_start(&p_Sound);
    }
}

void MP3AudioSource::Seek(uint32_t samplePosition)
{
    m_SeekTo = samplePosition;
}

uint32_t MP3AudioSource::Position()
{
    return m_Position;
}

float MP3AudioSource::PositionF()
{
    return ((int)m_Position - (int)m_Mp3DecCtx.info.hz / 4) / (float)m_Mp3DecCtx.info.hz;
}

void MP3AudioSource::Commit(void* ptr, size_t size, size_t frameDivider, size_t nbChannels, size_t sampleRate)
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

void MP3AudioSource::UpdateCommit(size_t frameDivider, size_t nbChannels, size_t sampleRate)
{
    size_t maxByteSize = sampleRate * frameDivider * nbChannels;
    if (m_CurrentCommitSize >= maxByteSize)
    {
        m_AudioBuffer->Queue(m_CommitBufferPtr, m_CurrentCommitSize / frameDivider);
        m_CurrentCommitSize = 0;
    }
}
