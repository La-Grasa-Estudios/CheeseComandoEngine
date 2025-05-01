#include "WaveAudioSource.h"

#include "Core/Logger.h"

using namespace ENGINE_NAMESPACE;

struct WaveHeader
{
    char  chunkID[4] = { 'R','I','F','F' };       // Contains the letters "RIFF" in ASCII form
    int32_t  chunkSize;                            // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
    char  format[4] = { 'W','A','V','E' };       // Contains the letters "WAVE" in ASCII form

    char  subchunk1ID[4] = { 'f','m','t',' ' };   // Contains the letters "fmt " in ASCII form
    int32_t  subchunk1Size = 16;                  // 16 for PCM
    int16_t audioFormat;                          // PCM = 1 Values other than 1 indicate some form of compression
    int16_t numChannels;                          // Mono = 1, Stereo = 2, etc
    int32_t  sampleRate;                           // 8000, 44100, etc.
    int32_t  byteRate;                             // SampleRate * NumChannels * BitsPerSample/8
    int16_t blockAlign;                           // NumChannels * BitsPerSample/8
    int16_t bitsPerSample;                        // 8 bits = 8, 16 bits = 16, etc

    char  subchunk2ID[4] = { 'd','a','t','a' };   // Contains the letters "data" in ASCII form
    int32_t  subchunk2Size;                        // This is the number of bytes in the data
};

WaveAudioSource::WaveAudioSource(SngFileStream* pStream, ma_engine* pEngine)
{
	m_AudioBuffer = NULL;
	m_Engine = pEngine;
	m_Stream = pStream;
}

WaveAudioSource::WaveAudioSource(const char* file, ma_engine* pEngine)
{
    m_AudioBuffer = NULL;
    m_Engine = pEngine;
    m_Stream = new SngFileStream(ZVFS::GetFileStream(file));

    if (!m_Stream->is_open())
    {
        return;
    }
}

WaveAudioSource::~WaveAudioSource()
{
    if (m_AudioBuffer) delete m_AudioBuffer;
    if (p_Params.IsReady) ma_sound_uninit(&p_Sound);
    if (m_Stream->is_self_contained()) delete m_Stream;
}

void WaveAudioSource::Rewind()
{
}

void WaveAudioSource::UpdateSource()
{

    WaveHeader* wavHeader = (WaveHeader*)this->waveHeaderBuffer;

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
        m_Stream->seekg(0);
        m_Stream->read(wavHeader, sizeof(WaveHeader));

        m_AudioBuffer = new Stratum::RawAudioBuffer(wavHeader->sampleRate, wavHeader->numChannels, wavHeader->bitsPerSample == 8 ? ma_format_u8 : ma_format_s16, wavHeader->byteRate / 4);

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
            Seek(0);
        }

        ma_sound_set_volume(&p_Sound, p_Params.Volume);
        ma_sound_set_pitch(&p_Sound, p_Params.Pitch);
        ma_sound_set_pan(&p_Sound, p_Params.Pan);
    }

    if(m_SeekTo != -1)
    {
        m_Position = m_SeekTo;
        size_t samplePosition = m_SeekTo * wavHeader->blockAlign;
        m_AudioBuffer->Clear();
        m_Stream->seekg(sizeof(WaveHeader) + samplePosition);
        m_SeekTo = -1;
    }

    size_t samples = 1;
    bool fl = true;

    while (m_AudioBuffer->CanWrite(wavHeader->blockAlign))
    {

        if (fl)
        {
            fl = false;
            samples = 0;
        }

        char buffer[4];
        memset(buffer, 0, sizeof(buffer));
        size_t readed = m_Stream->read(buffer, wavHeader->blockAlign);

        if (readed != wavHeader->blockAlign) /* normal eof or error condition */
        {
            break;
        }

        m_Position += 1;

        if (OnReadCallback)
            OnReadCallback(m_Position);

        if (m_SeekTo != -1)
        {
            m_Position = m_SeekTo;
            size_t samplePosition = m_SeekTo * wavHeader->blockAlign;
            m_Stream->seekg(sizeof(WaveHeader) + samplePosition);
            m_SeekTo = -1;
        }

        samples += readed;

        m_AudioBuffer->Queue(buffer, readed / wavHeader->numChannels);
    }

    if (!samples)
    {
        p_Params.ShouldPlay = false;
        p_Params.IsPlaying = false;
    }

    if (!ma_sound_is_playing(&p_Sound) || ma_sound_at_end(&p_Sound))
    {
        if (samples != 0)
        {
            ma_sound_start(&p_Sound);
        }
    }


}

void WaveAudioSource::Seek(uint32_t samplePosition)
{
    m_SeekTo = samplePosition;
}

uint32_t WaveAudioSource::Position()
{
	return m_Position;
}
