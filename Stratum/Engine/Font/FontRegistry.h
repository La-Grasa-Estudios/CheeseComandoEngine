#pragma once

#include <znmsp.h>
#include <unordered_map>

BEGIN_ENGINE

class Font;

class FontRegistry
{
public:
	FontRegistry(FontRegistry* other);
	FontRegistry() = default;
	~FontRegistry();
	// Load a font from a file
	void LoadFont(const std::string& fontName, const std::string& filePath);
	// Get a font by name
	Font* GetFont(const std::string& fontName);
	bool NeedsUpload(const std::string& fontName);
private:
	struct Entry
	{
		Font* font;
		bool isReadyInGpu;
	};
	std::unordered_map<std::string, Entry> mFonts;
	bool mOwnsFonts = true; // If true, the registry will delete the fonts when destroyed
};

END_ENGINE