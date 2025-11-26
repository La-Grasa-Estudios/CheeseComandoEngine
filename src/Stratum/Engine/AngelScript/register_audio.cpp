#include <cassert>
#include <fstream>
#include <string>
#include <string_view>
#include <glm/ext.hpp>

#include <Sound/AudioSourceBase.h>
#include <Sound/AudioEngine.h>
#include <VFS/ZVFS.h>

#include <AngelScript/AngelScript.h>
#include <angelscript.h>

using namespace ENGINE_NAMESPACE;

struct AudioObj
{
	Ref<AudioSourceBase> audioSource;
	int refCount = 0;
};

static void AudioConstructor(const std::string& file)
{
	
}

static void PlayOneShot(const std::string& path, float vol, float pitch, float pan)
{
	if (!ZVFS::Exists(path.c_str()))
	{
		return;
	}
	AudioEngine::s_Instance->PlayOneShot(path, vol, pitch, pan);
}

void as_RegisterAudio(asIScriptEngine* engine)
{
	engine->SetDefaultNamespace("Audio");
	engine->RegisterGlobalFunction("void playOneShot(const string& in path, float vol = 1.0f, float pitch = 1.0f, float pan = 0.5f)",
		asFUNCTION(PlayOneShot), asCALL_CDECL);
	engine->SetDefaultNamespace("");
}