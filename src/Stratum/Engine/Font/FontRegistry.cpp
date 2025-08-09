#include "FontRegistry.h"

#include "Font.h"

using namespace ENGINE_NAMESPACE;

FontRegistry::FontRegistry(FontRegistry* other)
{
	other->mOwnsFonts = false;
	mFonts = other->mFonts;

	for (auto kp : mFonts)
	{
		kp.second.isReadyInGpu = false;
	}
}

FontRegistry::~FontRegistry()
{
	if (mOwnsFonts)
	{
		for (auto& kp : mFonts)
		{
			delete kp.second.font;
		}
		mFonts.clear();
	}
}

void FontRegistry::LoadFont(const std::string& fontName, const std::string& filePath)
{
	if (mFonts.contains(fontName))
		return;

	Entry newEntry{};

	newEntry.font = new Font(filePath);
	newEntry.isReadyInGpu = false;

	mFonts[fontName] = newEntry;
}

Font* FontRegistry::GetFont(const std::string& fontName)
{
	if (auto kp = mFonts.find(fontName); kp != mFonts.end())
	{
		return kp->second.font;
	}
	return nullptr;
}

bool FontRegistry::NeedsUpload(const std::string& fontName)
{
	if (auto kp = mFonts.find(fontName); kp != mFonts.end())
	{
		if (kp->second.isReadyInGpu)
			return false;
		kp->second.isReadyInGpu = true;
		return true;
	}
	return false;
}
