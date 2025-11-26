#include "AudioEngine.h"

#include "MP3AudioSource.h"

#include <Core/Logger.h>

using namespace ENGINE_NAMESPACE;

AudioEngine::AudioEngine()
{

}

void AudioEngine::Init()
{

    m_AudioThreadRun.store(true);

    pAudioThread = new std::thread(
    [this]{
        ma_engine_config config = ma_engine_config_init();
        config.sampleRate = 44100;
        ma_engine_init(&config, &m_Engine);
        while (m_AudioThreadRun.load())
        {
            Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
        for (int i = 0; i < MAX_SOURCES; i++)
        {
            m_Sources[i] = NULL;
        }
        ma_engine_uninit(&m_Engine);
    });
    s_Instance = this;
}

void AudioEngine::Shutdown()
{
    m_AudioThreadRun.store(false);
    pAudioThread->join();
}

void AudioEngine::Update()
{
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        if (m_Sources[i])
        {
            m_Sources[i]->UpdateSource();
            m_Sources[i]->InternalUpdate();

            auto source = m_Sources[i].get();

            if (source->Removed || (!source->IsPlaying() && source->p_Params.RemoveOnFinish))
            {
                if (source->p_Params.IsReady)
                {
                    ma_node_detach_all_output_buses(&source->p_Sound);
                }
                m_Sources[i] = NULL;
            }
        }
    }
}

void AudioEngine::AddSource(Ref<AudioSourceBase> source)
{
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        if (!m_Sources[i])
        {
            m_Sources[i] = source;
            return;
        }
    }
}

void AudioEngine::PlayOneShot(const std::string& path, float volume, float pitch, float pan)
{
	Ref<AudioSourceBase> source = nullptr;
    if (path.ends_with("mp3"))
    {
		source = CreateRef<MP3AudioSource>(path.c_str(), this->GetEngine());
    }
	source->SetPan(pan);
	source->SetPitch(pitch);
	source->SetVolume(volume);
    this->AddSource(source);
    source->Play();
    source->p_Params.RemoveOnFinish = true;
}

void AudioEngine::StopAll()
{
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        if (m_Sources[i])
        {
            m_Sources[i]->Stop();
            m_Sources[i] = NULL;
        }
    }
}

ma_engine* AudioEngine::GetEngine()
{
    return &m_Engine;
}
