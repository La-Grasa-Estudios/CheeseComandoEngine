#pragma once

#include "znmsp.h"

#include <string_view>
#include <array>
#include <glm/ext.hpp>

BEGIN_ENGINE

struct CharRectangle
{
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
};

struct CharGlyph
{
	int advance_x = 0; // Horizontal advance for the glyph
	int advance_y = 0; // Vertical advance for the glyph
	float bearing_x = 0.0f; // Horizontal bearing for the glyph
	float bearing_y = 0.0f; // Vertical bearing for the glyph
	CharRectangle rect; // Rectangle defining the glyph's bitmap area
};

class Font
{
public:
	Font(const std::string_view& path);
	~Font();

	CharGlyph* GetGlyph(wchar_t c);
	char* GetBuffer() const;
	glm::ivec2 GetFontAtlasSize() const;

	void SetDescriptorHandle(uint32_t handle);
	uint32_t GetDescriptorHandle() const;
	bool IsFontReady() const;
	void WaitForFontReady() const;

private:

	char* mBuffer = nullptr; // Pointer to the font data buffer
	glm::ivec2 mResultSize;
	CharGlyph mGlyphs[0xFFFF];
	uint32_t mDescriptorHandle;
	bool mIsFontReady = false;

};

END_ENGINE