#include "TextBatcher.h"

#include <Font/Font.h>

using namespace ENGINE_NAMESPACE;

TextBatcher::TextBatcher(Font* font, SpriteBatch* batch)
{
	mFont = font;
	mBatch = batch;
}

TextBatcher::~TextBatcher()
{
	
}

void TextBatcher::SetParameters(const TextBatcherParameters& parameters)
{
	mParameters = parameters;
}

void TextBatcher::DrawText(const std::wstring& text, const glm::vec2& position, const glm::mat4& model)
{
	if (!mFont || !mBatch)
		return;

	static int frameIndex = 0;

	frameIndex++;

	frameIndex %= text.size();

	float baseScale = (1.0f / 32.0f);
	float scale = baseScale * mParameters.fontSize;

	glm::vec3 coords = { position.x, position.y, 0.0f };

	bool doWrap = false;
	bool wordComplete = false;
	uint32_t wordSize = 0;
	uint32_t idx = 0;
	uint32_t i = 0;

	while (i < text.size())
	{
		if (!wordComplete)
		{
			wordSize = 0;
			idx = i;
		}

		wchar_t termination = ' ';

		while (!wordComplete)
		{
			wchar_t c = text[i++];
			if (c == L' ' || c == L'\n' || c == L'\0')
			{
				wordComplete = true;
				termination = c;
				break;
			}
			wordSize++;
		}

		float wordLenght = 0.0f;

		for (uint32_t k = 0; k < wordSize; k++)
		{
			wchar_t c = text[k + idx];
			CharGlyph* glyph = mFont->GetGlyph(c);
			wordLenght += (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
		}

		if ((mParameters.wrapText && wordLenght + coords.x > mParameters.maxWidth) || doWrap)
		{
			float mult = (baseScale * mParameters.fontSize * mParameters.lineHeight);
			doWrap = false;
			// Wrap text to the next line
			coords.x = position.x;
			coords.y -= mFont->GetGlyph(L'l')->rect.h * mult + mult; // Move to the next line
			wordComplete = false;
		}

		while (wordSize > 0)
		{
			wchar_t c = text[idx];
			CharGlyph* glyph = mFont->GetGlyph(c);

			if (glyph)
			{
				SpriteBatch::SpriteInstance instance{};

				glm::mat4 model1 = glm::identity<glm::mat4>();

				glm::vec3 pos = coords;
				pos.x += glyph->bearing_x * scale;
				pos.y += glyph->bearing_y * scale;

				model1 = glm::translate(model1, pos);
				model1 = model * model1;

				instance.rect.position = { glyph->rect.x, glyph->rect.y };
				instance.rect.size = { glyph->rect.w, glyph->rect.h };
				instance.texture = mFont->GetDescriptorHandle();
				instance.RenderSize = glm::vec2(instance.rect.size) * scale;
				instance.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				instance.transform = glm::translate(glm::identity<glm::mat4>(), pos);
				instance.UserData = (2 << 1);
				instance.scaleWithRenderSize = false;

				SpriteBatch::SpriteInstance instance2 = instance;

				instance2.color = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
				instance2.transform = glm::translate(instance.transform, glm::vec3(mParameters.fontSize / 16.0f, -mParameters.fontSize / 16.0f, 0.0f));

				mBatch->DrawSprite(instance2);
				mBatch->DrawSprite(instance);

				coords.x += (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
			}

			wordSize--;
			idx++;
		}

		wordComplete = false;

		if (termination == ' ')
		{
			coords.x += 12 * scale;
		}

		if ((coords.x > mParameters.maxWidth && mParameters.wrapText) || termination == L'\n')
		{
			doWrap = true;
		}
	}
}

glm::vec2 TextBatcher::GetStringSize(const std::wstring& text) const
{
	float size_x = 0.0f;
	float line_x = 0.0f;
	float size_y = mParameters.fontSize + 1.0f;

	for (int i = 0; i < text.size(); i++)
	{
		CharGlyph* glyph = mFont->GetGlyph(text[i]);
		if (glyph)
		{
			float charSize = glyph->advance_x + mParameters.letterSpacing;
			size_x += charSize;
			line_x += charSize;
			if (line_x > mParameters.maxWidth && mParameters.wrapText)
			{
				line_x = 0.0f;
				size_y += mParameters.fontSize * mParameters.lineHeight + 1.0f; // Move to the next line
			}
		}
		else
		{
			size_x += mParameters.fontSize + mParameters.letterSpacing; // Fallback for unknown characters
		}
	}

	return glm::vec2(size_x, size_y);
}
