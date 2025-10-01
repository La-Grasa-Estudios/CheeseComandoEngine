#pragma once

#include <znmsp.h>

#define DECLARE_UI_COMPONENT_RENDERER(type, rendertype) \
namespace Internal { \
    struct internalUiComponent##type##link { \
        internalUiComponent##type##link() { \
            Stratum::SceneUI::AddComponentRenderer(UIComponentType::type, &RenderUI_##type, #rendertype); \
        } \
    }; \
    static internalUiComponent##type##link objReg##type; \
	static void* dummy##type = &objReg##type; \
}

class RendererFuncs
{
public:
    static void Init();
};