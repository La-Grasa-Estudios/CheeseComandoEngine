#pragma once

#include "Entity/ECS.h"

#include <json/json.hpp>

BEGIN_ENGINE

struct AppUIEvent
{
	std::string EventName;
	std::string ElementName;
	std::string PanelName;
};

namespace Render
{
	class GraphicsCommandBuffer;
	class CopyCommandBuffer;
}

struct RenderQueue2D;
struct Render2DInstance;
class Scene;

enum class UIComponentType
{
	INVALID = -1,
	RECT,
	LABEL,
	BUTTON,
};

enum class UINextElement
{
	INVALID = -1,
	LEFT,
	RIGHT,
	TOP,
	BOTTOM
};

struct UIButton
{
	std::string OnClick;
	glm::vec4 HoveredColor = glm::vec4(1.0);
	glm::vec4 UnhoveredColor = glm::vec4(1.0);
};

struct UILabel
{
	std::wstring Text;
	float TextAlignment;
};

enum UIPanelTransitionState
{
	IDLE,
	SHOWING,
	HIDING
};

struct UIComponent;

class UITransitionModule
{
public:
	virtual glm::mat4 GetMatrix(UIComponent* component, float p) = 0;
	virtual glm::vec4 GetColor(UIComponent* component, float p) = 0;
};

struct UIComponent
{
	UIComponentType Type;
	UIComponent* Parent = nullptr;
	UINextElement NextElement = UINextElement::INVALID;
	std::string Name;
	std::string Anchor;
	std::string Font = "Roboto";
	float Width = 0.0f;
	float Height = 0.0f;
	float PaddingLeft = 0.0f;
	float PaddingRight = 0.0f;
	float PaddingTop = 0.0f;
	float PaddingBottom = 0.0f;
	float TransformX = 0.0f;
	float TransformY = 0.0f;
	float FontSize = 64.0f;
	bool IsBackground = false; // Ignore when placing layout
	glm::vec2 EffectiveSize = {};
	glm::vec2 Position = {};
	glm::vec4 BgColor = glm::vec4(1.0f);
	glm::vec4 FgColor = glm::vec4(1.0f);
	uint8_t CameraLayer = 0;
	uint32_t RenderLayer = 0;
	UIButton Button;
	UILabel Label;
	std::vector<Ref<UIComponent>> Components;

	int32_t Background = -1;
	int32_t Foreground = -1;

	glm::vec2 BoundingBoxExtends = {};
	glm::vec2 LayoutBoundingBoxExtends = {};

	float TransitionOffset = 0.0f;
	float TransitionStartTimer = 0.0f;
	float TransitionProgress = 0.0f;
	float TransitionLength = 1.0f;
	UIPanelTransitionState TransitionState = UIPanelTransitionState::IDLE;
	UITransitionModule* TransitionModuleIn = nullptr;
	UITransitionModule* TransitionModuleOut = nullptr;
	glm::vec4 TransitionColor = glm::vec4(1.0f);

	std::string PanelName;
	std::string OnHover;

	std::string PadUp;
	std::string PadDown;
	std::string PadLeft;
	std::string PadRight;

	bool Hovered = false;

	float GetRootEM()
	{
		if (Parent)
			return Parent->GetRootEM();
		return FontSize;
	}

	float GetPositionX()
	{
		float offset = 0.0f;
		if (Parent)
			offset = Parent->GetPositionX();
		return offset + Position.x;
	}
	float GetPositionY()
	{
		float offset = 0.0f;
		if (Parent)
			offset = Parent->GetPositionY();
		return offset + Position.y;
	}
};

struct UIPanel
{
	bool Active = false;

	std::string Name;
	std::string Path;
	std::string PadDefault;
	glm::ivec2 PanelResolution;
	std::vector<Ref<UIComponent>> Roots;
};

class UITransitionFactoryInterface
{
public:
	UITransitionModule* pModule;
};

template<typename T>
class UITransitionFactory : public UITransitionFactoryInterface
{
	public:
	UITransitionFactory()
	{
		pModule = new T();
	}
};

class SceneUI
{

public:

	SceneUI(Scene* scene);
	~SceneUI();

	void CreateUIPanel(const std::string& panelName, const std::string& jsonFile);
	void ShowUIPanel(const std::string& panelName);
	void HideUIPanel(const std::string& panelName);

	void Update();

	void PreRender(Render::CopyCommandBuffer* pCmd);
	void Render(RenderQueue2D* ppRenderQueues);
	void OnTextRender(Render2DInstance* pInstance);

	UIComponent* FindObject(const std::string& panelName, const std::string& name);
	bool IsMouseHidden();

	bool HideMouseWithController = true;

	UIComponent* hoveredComponent = nullptr;

private:

	bool mIsMouseHidden = false;

	void CalculateBBoxes(UIPanel& panel);
	void CalculateLayout(UIComponent* component);
	bool TransitionFinished(UIPanel& panel);

	float GetComponentWidth(UIComponent* component);
	float GetComponentHeight(UIComponent* component);

	glm::vec2 ComputeComponentBBox(UIComponent* component);

	void ParseTree(nlohmann::json& json, UIComponent* Parent, UIPanel& panel);

	float ParseUnits(const std::string& str, float relative, float rem, float em);
	void ParseVector(float* dest, int nbComps, nlohmann::json& json);

	Scene* mScene;
	ECS::edict_t mTextEntity;

	std::unordered_map<std::string, UIPanel> mPanels;
	std::unordered_map<std::string, Ref<UITransitionFactoryInterface>> mTransitions;

	std::vector<std::string> mFocusedPanels;

};

END_ENGINE