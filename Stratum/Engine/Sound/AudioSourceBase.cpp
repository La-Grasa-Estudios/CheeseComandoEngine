#include "AudioSourceBase.h"

using namespace ENGINE_NAMESPACE;

AudioSourceBase::AudioSourceBase()
{
	p_Sound = {};
	p_Params.Pan = 0.0f;
}

void AudioSourceBase::Play()
{
	p_Params.DoRewind = true;
	p_Params.IsPlaying = true;
	p_Params.ShouldPlay = true;
}

void AudioSourceBase::Stop(bool fully, bool fadeOut, uint32_t fadeOutMillis)
{
	p_Params.ShouldPlay = false;
	p_Params.IsPlaying = false;
	p_Params.DoStop = true;
}

void AudioSourceBase::SetVolume(float volume)
{
	p_Params.Volume = volume;
}

float AudioSourceBase::GetVolume()
{
	return p_Params.Volume;
}

void AudioSourceBase::SetPan(float pan)
{
	p_Params.Pan = pan;
}

float AudioSourceBase::GetPan()
{
	return p_Params.Pan;
}

void AudioSourceBase::SetPitch(float pitch)
{
	p_Params.Pitch = pitch;
}

float AudioSourceBase::GetPitch()
{
	return p_Params.Pitch;
}

void AudioSourceBase::Seek(uint32_t samplePosition)
{
}

uint32_t AudioSourceBase::Position()
{
	return 0;
}

bool AudioSourceBase::IsPlaying()
{
	return p_Params.IsPlaying;
}

void AudioSourceBase::AttachToNode(ma_node* pNode, uint32_t index, uint32_t inputIndex)
{
	if (!p_Params.IsReady) return;
	ma_node_detach_all_output_buses(&p_Sound);
	ma_node_attach_output_bus(pNode, index, &p_Sound, inputIndex);
}
