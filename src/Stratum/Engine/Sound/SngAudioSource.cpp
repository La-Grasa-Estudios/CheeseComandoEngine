#include "SngAudioSource.h"

#include "VFS/ZVFS.h"

#include "MP3AudioSource.h"
#include "WaveAudioSource.h"
#include "Core/Logger.h"

using namespace ENGINE_NAMESPACE;

SngAudioSource::SngAudioSource(const char* file, ma_engine* pEngine)
{

	m_pSngFile = 0;

	SngFile* pFile = new SngFile(file);

	if (!pFile->Metadata.contains("container"))
	{
		delete pFile;
		Z_WARN("File {} is not a valid .sng file!", file);
		return;
	}

	m_pSngFile = pFile;

	std::string songContainer = "song.";

	songContainer.append(pFile->Metadata["container"]);

	SngFileMetadata* pMeta = pFile->Files[songContainer].get();
	SngFileStream* pSngStream = new SngFileStream(*pFile, pMeta);

	this->p_SngFileMetadata = pMeta;

	if (songContainer.ends_with("mp3"))
	{
		m_AudioSource = CreateRef<MP3AudioSource>(pSngStream, pEngine);
	}
	if (songContainer.ends_with("wav"))
	{
		m_AudioSource = CreateRef<WaveAudioSource>(pSngStream, pEngine);
	}

	if (pFile->Metadata.contains("isLooped"))
	{
		std::string looped = pFile->Metadata["isLooped"];
		std::string start = pFile->Metadata["loopStart"];
		std::string end = pFile->Metadata["loopEnd"];

		if (looped.compare("true") == 0)
		{
			this->DoesLoop = true;
		}
		else
		{
			this->DoesLoop = false;
		}

		this->LoopStart = std::atoi(start.c_str());
		this->LoopEnd = std::atoi(end.c_str());

		m_AudioSource->OnReadCallback = [&](int64_t pos)
			{
				if (pos > LoopEnd)
				{
					m_AudioSource->Seek(LoopStart);
				}
			};
	}

}

SngAudioSource::~SngAudioSource()
{
	if (m_AudioSource) m_AudioSource->Stop();
	if (m_pSngFile) delete m_pSngFile;
}

void SngAudioSource::Play()
{
	if (!m_AudioSource) return;
	m_AudioSource->Play();
}

void SngAudioSource::Stop(bool fully, bool fadeOut, uint32_t fadeOutMillis)
{
	if (!m_AudioSource) return;
	m_AudioSource->Stop(fully, fadeOut, fadeOutMillis);
}

void SngAudioSource::SetVolume(float volume)
{
	if (!m_AudioSource) return;
	m_AudioSource->SetVolume(volume);
}

void SngAudioSource::SetPan(float pan)
{
	if (!m_AudioSource) return;
	m_AudioSource->SetPan(pan);
}

void SngAudioSource::SetPitch(float pitch)
{
	if (!m_AudioSource) return;
	m_AudioSource->SetPitch(pitch);
}

void SngAudioSource::Rewind()
{
	if (!m_AudioSource) return;
	m_AudioSource->Rewind();
}

void SngAudioSource::UpdateSource()
{
	if (!m_AudioSource) return;
	m_AudioSource->UpdateSource();
}

bool SngAudioSource::IsPlaying()
{
	if (!m_AudioSource) return false;
	return m_AudioSource->IsPlaying();
}

void SngAudioSource::Seek(uint32_t samplePosition)
{
	if (!m_AudioSource) return;
	m_AudioSource->Seek(samplePosition);
}

uint32_t SngAudioSource::Position()
{
	if (!m_AudioSource) 0;
	return m_AudioSource->Position();
}
