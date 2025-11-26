#include "RendererFuncs.h"
#include <Scene/SceneUI.h>
#include <Scene/RendererCommon.h>
#include <Core/Logger.h>

using namespace ENGINE_NAMESPACE;

void RenderUI_BUTTON(UIComponent* pComponent, Render2DInstance* pInstance)
{
	if (!pComponent->Hovered)
	{
		pInstance->batch.color = pComponent->Button.UnhoveredColor;
	}
	else
	{
		pInstance->batch.color = pComponent->Button.HoveredColor;
	}
}
DECLARE_UI_COMPONENT_RENDERER(BUTTON, rect);

void RenderUI_CHECKBOX(UIComponent* pComponent, Render2DInstance* pInstance)
{
	RenderUI_BUTTON(pComponent, pInstance);
	glm::vec2 size = {};
	if (!pComponent->Checkbox.value)
	{
		if (pComponent->uimg.states.contains("unchecked"))
		{
			auto& state = pComponent->uimg.states["unchecked"];
			auto offset = glm::vec3(state.offset_x, state.offset_y, 0.0f);
			auto scale = glm::vec3(pComponent->Width, pComponent->Height, 0.0f) / 100.0f;
			offset *= scale;
			pInstance->batch.texture = state.texture;
			pInstance->batch.transform = glm::translate(pInstance->batch.transform, offset);
			size = { state.render_width, state.render_height };
		}
	}
	else
	{
		if (pComponent->uimg.states.contains("checked"))
		{
			auto& state = pComponent->uimg.states["checked"];
			auto offset = glm::vec3(state.offset_x, state.offset_y, 0.0f);
			auto scale = glm::vec3(pComponent->Width, pComponent->Height, 0.0f) / 100.0f;
			offset *= scale;
			pInstance->batch.texture = state.texture;
			pInstance->batch.transform = glm::translate(pInstance->batch.transform, offset);
			size = { state.render_width, state.render_height };
		}
	}
	size = (size / 100.0f) * glm::vec2(pComponent->Width, pComponent->Height);
	pInstance->batch.RenderSize = size;
}
DECLARE_UI_COMPONENT_RENDERER(CHECKBOX, rect);

void RenderUI_LABEL(UIComponent* pComponent, Render2DInstance* pInstance)
{
	// No special rendering for labels yet
}
DECLARE_UI_COMPONENT_RENDERER(LABEL, text);

void RendererFuncs::Init()
{
	Z_INFO("UI Renderer vtable initialized");
}