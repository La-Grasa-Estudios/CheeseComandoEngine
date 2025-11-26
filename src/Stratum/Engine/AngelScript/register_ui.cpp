#include <cassert>
#include <fstream>
#include <string>
#include <string_view>
#include <glm/ext.hpp>

#include <Scene/SceneUI.h>
#include <Event/EventBus.h>

#include <AngelScript/AngelScript.h>
#include <angelscript.h>

using namespace ENGINE_NAMESPACE;

static UIComponent* GetParent(UIComponent& self)
{
	return self.Parent;
}

static UIComponent* FindByName(SceneUI& scene, const std::string& panelId, const std::string& name)
{
	return scene.FindObject(panelId, name);
}

static void HidePanel(SceneUI& scene, const std::string& panelId)
{
	scene.HideUIPanel(panelId);
}

static void ShowPanel(SceneUI& scene, const std::string& panelId)
{
	scene.ShowUIPanel(panelId);
}

static void InvokeEvent(SceneUI& scene, const std::string& event, UIComponent* component)
{
	auto e = AppUIEvent{ .EventName = event };
	if (component)
	{
		e.ElementName = component->Name;
		e.PanelName = component->PanelName;
	}
	EventBus::InvokeEvent(e);
}

static void AddEventHandler(UIComponent* self, const std::string& eventName, asIScriptFunction* func)
{
	self->EventCallbacks[eventName] = func;
}

static void RemoveEventHandler(UIComponent& self, const std::string& eventName)
{
	if (self.EventCallbacks.contains(eventName))
		self.EventCallbacks.erase(eventName);
}

void as_RegisterUI(asIScriptEngine* engine)
{
	engine->SetDefaultAccessMask(AS_UI_SCRIPT_MASK);

	engine->RegisterEnum("UIComponentType");
	engine->RegisterEnumValue("UIComponentType", "INVALID", (int)UIComponentType::INVALID);
	engine->RegisterEnumValue("UIComponentType", "BUTTON", (int)UIComponentType::BUTTON);
	engine->RegisterEnumValue("UIComponentType", "CHECKBOX", (int)UIComponentType::CHECKBOX);
	engine->RegisterEnumValue("UIComponentType", "RECT", (int)UIComponentType::RECT);

	engine->RegisterObjectType("UIComponent", sizeof(UIComponent), asOBJ_REF | asOBJ_NOCOUNT);
	engine->RegisterFuncdef("void OnEvent(UIComponent@)");
	engine->RegisterObjectProperty("UIComponent", "UIComponentType Type", asOFFSET(UIComponent, Type));
	engine->RegisterObjectProperty("UIComponent", "bool boolValue", asOFFSET(UIComponent, Checkbox.value));
	engine->RegisterObjectProperty("UIComponent", "string userData", asOFFSET(UIComponent, UserData));
	engine->RegisterObjectProperty("UIComponent", "vec4 bgColor", asOFFSET(UIComponent, BgColor));
	engine->RegisterObjectProperty("UIComponent", "vec4 fgColor", asOFFSET(UIComponent, FgColor));
	engine->RegisterObjectProperty("UIComponent", "vec4 unhoveredColor", asOFFSET(UIComponent, Button.UnhoveredColor));
	engine->RegisterObjectProperty("UIComponent", "vec4 hoveredColor", asOFFSET(UIComponent, Button.HoveredColor));
	//engine->RegisterObjectProperty("UIComponent", "bool text", asOFFSET(UIComponent, Label.Text));
	engine->RegisterObjectMethod("UIComponent", "UIComponent@ getParent()", asFUNCTION(GetParent), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("UIComponent", "void addEventHandler(const string &in event, OnEvent @handler)", asFUNCTION(AddEventHandler), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("UIComponent", "void removeEventHandler(const string &in event)", asFUNCTION(RemoveEventHandler), asCALL_CDECL_OBJFIRST);

	engine->RegisterObjectType("UIManager", 8, asOBJ_REF | asOBJ_NOCOUNT);
	engine->RegisterObjectMethod("UIManager", "UIComponent@ getElementByName(const string& in documentTitle, const string& in objectName) const", asFUNCTION(FindByName), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("UIManager", "void hideDocument(const string& in documentTitle)", asFUNCTION(HidePanel), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("UIManager", "void showDocument(const string& in documentTitle)", asFUNCTION(ShowPanel), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("UIManager", "void invokeEvent(const string& in eventName, UIComponent@ comp)", asFUNCTION(InvokeEvent), asCALL_CDECL_OBJFIRST);

	engine->SetDefaultAccessMask(AS_ALL_MASK);
}