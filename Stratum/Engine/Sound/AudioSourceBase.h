#pragma once

#include "miniaudio/miniaudio.h"
#include "znmsp.h"

#include <future>

BEGIN_ENGINE

struct AudioSourceParams
{
	bool IsPlaying = false;
	bool DoRewind = false;
	bool DoStop = false;
	bool IsReady = false;
	bool ShouldPlay = false;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	float Pan = 0.5f;
};

class AudioSourceBase
{

public:

	AudioSourceBase();

	virtual void Play();
	virtual void Stop(bool fully = false, bool fadeOut = false, uint32_t fadeOutMillis = 100);
	virtual void Rewind() {}

	virtual void SetVolume(float volume);
	virtual float GetVolume();

	virtual void SetPan(float pan);
	virtual float GetPan();

	virtual void SetPitch(float pitch);
	virtual float GetPitch();

	virtual bool IsPlaying();

	virtual void Seek(uint32_t samplePosition);
	virtual uint32_t Position();
	virtual float PositionF() { return 0.0f; }

	virtual void UpdateSource() {}
	virtual void AttachToNode(ma_node* pNode, uint32_t index, uint32_t inputIndex);
	
	bool Removed = false;

	std::function<void(int64_t)> OnReadCallback;

	std::string Path;

protected:

	AudioSourceParams p_Params;

	ma_sound p_Sound;

};

END_ENGINE