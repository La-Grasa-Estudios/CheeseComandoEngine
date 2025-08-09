#include "CharaRegistry.h"

#include "Common.h"

void Funkin::CharaRegistry::Init(Stratum::Scene* scene)
{
	sCharaSprites.clear();
	sScene = scene;
}

void Funkin::CharaRegistry::AddCharacter(const std::string& name)
{
	if (sCharaSprites.contains(name))
		return;

	auto chara = Stratum::CreateRef<CharaSprite>(sScene, GenerateAssetPath(C_CHARA_PATH_PREFIX, name, "json"));

	sCharaSprites[name] = chara;
}

void Funkin::CharaRegistry::Update()
{
	for (auto kp : sCharaSprites)
	{
		kp.second->Update();
	}
}

Funkin::CharaSprite* Funkin::CharaRegistry::GetCharacter(const std::string& name)
{
	if (sCharaSprites.contains(name))
		return sCharaSprites[name].get();

	return nullptr;
}
