#include "AudioSourceBase.h"
#include <glm/ext.hpp>
#include <kissfft/kiss_fft.h>

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

void AudioSourceBase::Pause()
{
	p_Params.IsPaused = true;
	p_Params.IsPlaying = false;
}

void AudioSourceBase::Resume()
{
	p_Params.IsPaused = false;
	p_Params.IsPlaying = true;
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

void AudioSourceBase::Seek(float timeInSeconds)
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

void AudioSourceBase::SetLooping(bool looping)
{
	p_Params.IsLooping = looping;
}

void AudioSourceBase::EnableWaveform(WaveformMode mode, uint32_t stepSize, float decayFactor)
{
	if (stepSize == 0) stepSize = 512;
	if (decayFactor <= 0.0f || decayFactor >= 1.0f) decayFactor = 0.95f;
	p_DecayFactor = decayFactor;
	uint32_t domainSize = 65536 / stepSize;
	p_FrequencyDomain.resize(domainSize);
	p_WaveformMode = mode;
}

void AudioSourceBase::InternalUpdate()
{
	if (p_FrequencyDomain.data())
	{
		for (int i = 0; i < p_FrequencyDomain.size(); i++)
		{
			p_FrequencyDomain[i] *= p_DecayFactor;
		}
	}
}

void AudioSourceBase::OnSampleWrite(int16_t* pSamples, uint32_t sampleCount)
{
	if (p_FrequencyDomain.size() > 0)
	{
		for (uint32_t i = 0; i < sampleCount; i++)
		{
			

			const size_t sampleSize = 256;
			uint32_t stepSize = 65536 / p_FrequencyDomain.size();
			float stepPow = stepSize / 512.0f;

			if (p_WaveformMode == WaveformMode::AMPLITUDE_DOMAIN)
			{
				auto sample = pSamples[i];
				float v = glm::clamp(glm::abs(sample) / 32768.0f, 0.0f, 1.0f);
				uint32_t index = (v * 65536) / stepSize;
				v = (v / 4.0f) / stepPow / sampleCount;
				float mult = 1.0f;
				if (index > 0)
				{
					mult -= 0.33f;
				}
				if (index < p_FrequencyDomain.size() - 1)
				{
					mult -= 0.33f;
					p_FrequencyDomain[index + 1] += v * mult;
				}
				if (index < p_FrequencyDomain.size())
					p_FrequencyDomain[index] += v * mult;
				if (index > 0)
				{
					p_FrequencyDomain[index - 1] += v * mult;
				}
			}
			else if (p_WaveformMode == WaveformMode::FREQUENCY_DOMAIN)
			{
				p_SampleBuffer.push_back(pSamples[i] / 32768.0f);
			}

			if (p_SampleBuffer.size() >= sampleSize)
			{
				kiss_fft_cfg cfg = kiss_fft_alloc(sampleSize, 1, nullptr, nullptr);

				kiss_fft_cpx in[sampleSize], out[sampleSize];

				for (size_t i = 0; i < p_SampleBuffer.size(); ++i) {
					float window = 0.5f * (1.0f - cos(2.0f * glm::pi<float>() * i / (p_SampleBuffer.size() - 1))); // Hann
					p_SampleBuffer[i] *= window;
				}

				for (int j = 0; j < sampleSize; j++)
				{
					in[j].r = p_SampleBuffer[j];
					in[j].i = 0;
				}

				kiss_fft(cfg, in, out);

				for (size_t i = 0; i < p_FrequencyDomain.size(); ++i) {
					size_t fftIndex = i * (sampleSize / 2 / p_FrequencyDomain.size());
					float mag = sqrt(out[fftIndex].r * out[fftIndex].r + out[fftIndex].i * out[fftIndex].i);
					p_FrequencyDomain[i] += mag / stepPow / 6.0f / sampleCount; // or accumulate/smooth
				}

				free(cfg);
				p_SampleBuffer.clear();
			}
		}
		/*
		
		for (uint32_t i = 0; i < sampleCount; i++)
		{
			auto sample = pSamples[i];
			float v = glm::clamp(glm::abs(sample) / 32768.0f, 0.0f, 1.0f);
			uint32_t index = (v * 65536) / stepSize;
			v = (v / 4.0f) / stepPow / sampleCount;

			

			float mult = 1.0f;

			if (index > 0)
			{
				mult -= 0.33f;
			}

			if (index < p_FrequencyDomain.size() - 1)
			{
				mult -= 0.33f;
				p_FrequencyDomain[index + 1] += v * mult;
			}

			if (index < p_FrequencyDomain.size())
				p_FrequencyDomain[index] += v * mult;

			if (index > 0)
			{
				p_FrequencyDomain[index - 1] += v * mult;
			}
		}
		*/
	}
}
