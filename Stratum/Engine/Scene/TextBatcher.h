#pragma once

#include "TextRenderer.h"

BEGIN_ENGINE

class Font;
class SceneResources;

namespace Render
{
	class CopyCommandBuffer;
	class GraphicsCommandBuffer;
}

struct TextBatcherParameters
{
	float lineHeight = 1.0f; // Height of a line in text
	float letterSpacing = 0.0f; // Spacing between letters
	float maxWidth = 0.0f; // Maximum width of a line before wrapping
	float fontSize = 16.0f; // Size of the font in pixels
	bool wrapText = true; // Whether to wrap text to the next line if it exceeds maxWidth
};

class TextBatcher
{
public:

	TextBatcher(Font* font, SceneResources* resources);
	~TextBatcher();

	void SetParameters(const TextBatcherParameters& parameters);

	void Begin();
	void DrawText(const std::wstring& text, const glm::vec2& position, const glm::mat4& model);
	void End(Render::CopyCommandBuffer* pCmdBuffer);
	void Render(Render::GraphicsCommandBuffer* pCmdBuffer);
	glm::vec2 GetStringSize(const std::wstring& text) const;

private:

	TextRenderer* mBatch;
	Font* mFont;
	TextBatcherParameters mParameters;
};

END_ENGINE