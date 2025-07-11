#include "TextBatcher.h"

#include <Font/Font.h>

using namespace ENGINE_NAMESPACE;

TextBatcher::TextBatcher(SpriteBatch* batch)
{
	mBatch = batch;
}

TextBatcher::~TextBatcher()
{
	
}

void TextBatcher::SetParameters(const TextBatcherParameters& parameters)
{
	mParameters = parameters;
}

void TextBatcher::DrawText(const std::wstring& text, const glm::vec2& position, const glm::mat4& model, glm::vec4 color, bool gui)
{
	if (!mParameters.font || !mBatch || text.empty())
		return;

	static int frameIndex = 0;

	frameIndex++;

	frameIndex %= text.size();

	float baseScale = (1.0f / 32.0f);
	float scale = baseScale * mParameters.fontSize;

	glm::vec3 coords = { position.x, position.y, 0.0f };
	float size_x = 0.0f;

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
			CharGlyph* glyph = mParameters.font->GetGlyph(c);
			wordLenght += (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
		}

		if ((mParameters.wrapText && wordLenght + size_x > mParameters.maxWidth) || doWrap)
		{
			float mult = (baseScale * mParameters.fontSize * mParameters.lineHeight);
			doWrap = false;
			// Wrap text to the next line
			size_x = 0.0f;
			coords.x = position.x;
			coords.y -= mParameters.font->GetGlyph(L'l')->rect.h * mult + mult; // Move to the next line
			wordComplete = false;
		}

		while (wordSize > 0)
		{
			wchar_t c = text[idx];
			CharGlyph* glyph = mParameters.font->GetGlyph(c);

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
				instance.texture = mParameters.font->GetDescriptorHandle();
				instance.RenderSize = glm::vec2(instance.rect.size) * scale;
				instance.color = color;
				instance.transform = glm::translate(glm::identity<glm::mat4>(), pos);
				instance.UserData = (2 << 1) | (int)(gui);
				instance.scaleWithRenderSize = false;

				SpriteBatch::SpriteInstance instance2 = instance;

				instance2.color = instance.color * glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
				instance2.transform = glm::translate(instance.transform, glm::vec3(mParameters.fontSize / 16.0f, -mParameters.fontSize / 16.0f, 0.0f));

				mBatch->DrawSprite(instance2);
				mBatch->DrawSprite(instance);

				float advance = (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
				size_x += advance;
				coords.x += advance;
			}

			wordSize--;
			idx++;
		}

		wordComplete = false;

		if (termination == ' ')
		{
			coords.x += 12 * scale;
			size_x += 12 * scale;
		}

		if ((size_x > mParameters.maxWidth && mParameters.wrapText) || termination == L'\n')
		{
			doWrap = true;
		}
	}
}

glm::vec3 TextBatcher::GetStringSize(const std::wstring& text) const
{
	if (!mParameters.font || !mBatch || text.empty())
		return glm::vec3(0.0f);

	static int frameIndex = 0;

	frameIndex++;

	frameIndex %= text.size();

	float baseScale = (1.0f / 32.0f);
	float scale = baseScale * mParameters.fontSize;

	glm::vec3 coords = { 0.0f, 0.0f, 0.0f };
	float line_size_x = 0.0f;
	float size_x = 0.0f;
	float size_y = 0.0f;
	float lineWraps = 0.0f;

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
			CharGlyph* glyph = mParameters.font->GetGlyph(c);
			wordLenght += (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
		}

		if ((mParameters.wrapText && wordLenght + line_size_x > mParameters.maxWidth) || doWrap)
		{
			float mult = (baseScale * mParameters.fontSize * mParameters.lineHeight);
			doWrap = false;
			// Wrap text to the next line
			line_size_x = 0.0f;
			coords.x = 0.0f;
			coords.y += mParameters.font->GetGlyph(L'l')->rect.h * mult + mult; // Move to the next line
			size_y += mParameters.font->GetGlyph(L'l')->rect.h * mult + mult; // Move to the next line
			wordComplete = false;
			lineWraps++;
		}

		while (wordSize > 0)
		{
			wchar_t c = text[idx];
			CharGlyph* glyph = mParameters.font->GetGlyph(c);

			if (glyph)
			{
				glm::vec2 size = glm::vec2{ glyph->rect.w, glyph->rect.h } * scale;
				size_x = glm::max(size_x, size.x + line_size_x);
				size_y = glm::max(size_y, size.y + coords.y);

				float advance = (glyph->advance_x >> 6) * scale + mParameters.letterSpacing * scale;
				line_size_x += advance;
				coords.x += advance;
			}

			wordSize--;
			idx++;
		}

		wordComplete = false;

		if (termination == ' ')
		{
			coords.x += 12 * scale;
			line_size_x += 12 * scale;
		}

		if ((line_size_x > mParameters.maxWidth && mParameters.wrapText) || termination == L'\n')
		{
			doWrap = true;
		}
	}

	return { size_x, size_y, lineWraps };
}
