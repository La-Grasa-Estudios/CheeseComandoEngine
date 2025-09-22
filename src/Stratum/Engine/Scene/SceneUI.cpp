
#include "SceneUI.h"

#include "RendererCommon.h"
#include "TextBatcher.h"
#include "UI/RendererFuncs.h"

#include <Event/EventBus.h>
#include <Input/Input.h>
#include <Scene/Scene.h>
#include <VFS/ZVFS.h>
#include <VFS/base64.hpp>
#include <Util/StrUtil.h>
#include <Util/Globals.h>

#include <queue>
#include <zlib/izlibstream.h>
#include <VFS/stb/stb_image.h>

using namespace ENGINE_NAMESPACE;

template<typename T>
static T GetJsonValue(nlohmann::json& json, const std::string& key, const T& defaultValue)
{
	if (json.contains(key))
	{
		return json[key].get<T>();
	}
	return defaultValue;
}

static float easeInOutCubic(float x) {
	return x < 0.5 ? 4 * x * x * x : 1 - glm::pow(-2 * x + 2, 3) / 2;
}

static float easeOutElastic(float x) {
	const float  c4 = (2 * glm::pi<float>()) / 3;
	
	return x == 0
	  ? 0
	  : x == 1
	  ? 1
	  : glm::pow(2, -10 * x) * glm::sin((x * 10 - 0.75) * c4 * 1.0f) * 2.0f + 1;
}

static float easeOutBounce(float x) {
const float n1 = 7.5625;
const float d1 = 2.75;

if (x < 1 / d1) {
	return n1 * x * x;
}
 else if (x < 2 / d1) {
  return n1 * (x -= 1.5f / d1) * x + 0.75f;
}
 else if (x < 2.5 / d1) {
  return n1 * (x -= 2.25f / d1) * x + 0.9375f;
}
 else {
  return n1 * (x -= 2.625f / d1) * x + 0.984375f;
}
}

static struct AABB
{
	float x0;
	float y0;
	float x1;
	float y1;

	AABB(glm::vec2 pos, glm::vec2 extends)
	{
		x0 = pos.x;
		y0 = pos.y - extends.y;
		x1 = pos.x + extends.x;
		y1 = pos.y;
	}

	bool Overlap(const AABB& other) const
	{
		return other.x1 > x0 && other.x0 < x1 && other.y1 > y0 && other.y0;
	}

	bool PointInside(glm::vec2 point) const
	{
		return point.x > x0 && point.x < x1 && point.y > y0 && point.y < y1;
	}
};

class ScaleTransitionModule : public UITransitionModule
{
public:
	glm::mat4 GetMatrix(UIComponent* component, float p) override
	{
		p = easeInOutCubic(p);
		float s = glm::mix(0.0f, 1.0f, p);
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::scale(transform, glm::vec3(s, s, 1.0f));
		return transform;
	}
	glm::vec4 GetColor(UIComponent* component, float p) override
	{
		p = glm::max((p - 0.6f) / 0.4f, 0.0f);
		return glm::vec4(1.0f, 1.0f, 1.0f, p);
	}
};

class ScaleBounceTransitionModule : public UITransitionModule
{
public:
	glm::mat4 GetMatrix(UIComponent* component, float p) override
	{
		p = easeOutBounce(p);
		float s = glm::mix(0.0f, 1.0f, p);
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::scale(transform, glm::vec3(s, s, 1.0f));
		return transform;
	}
	glm::vec4 GetColor(UIComponent* component, float p) override
	{
		p = glm::max((p - 0.6f) / 0.4f, 0.0f);
		return glm::vec4(1.0f, 1.0f, 1.0f, p);
	}
};

class SlideRightBounceTransitionModule : public UITransitionModule
{
public:
	glm::mat4 GetMatrix(UIComponent* component, float p) override
	{
		p = easeOutBounce(p);
		float s = glm::mix(1.0f, 0.0f, p);
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, glm::vec3(s * 3000.0f, 0.0f, 1.0f));
		return transform;
	}
	glm::vec4 GetColor(UIComponent* component, float p) override
	{
		return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
};

class SlideRightTransitionModule : public UITransitionModule
{
public:
	glm::mat4 GetMatrix(UIComponent* component, float p) override
	{
		p = easeInOutCubic(p);
		float s = glm::mix(1.0f, 0.0f, p);
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, glm::vec3(s * 4000.0f, 0.0f, 1.0f));
		return transform;
	}
	glm::vec4 GetColor(UIComponent* component, float p) override
	{
		return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
};

class SlideLeftTransitionModule : public UITransitionModule
{
public:
	glm::mat4 GetMatrix(UIComponent* component, float p) override
	{
		p = easeInOutCubic(p);
		float s = glm::mix(1.0f, 0.0f, p);
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, glm::vec3(s * -4000.0f, 0.0f, 1.0f));
		return transform;
	}
	glm::vec4 GetColor(UIComponent* component, float p) override
	{
		return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
};

SceneUI::SceneUI(Scene* scene)
{
	mScene = scene;

	RendererFuncs::Init();

	auto entity = scene->EntityManager.CreateEntity();
	scene->Transforms.Create(entity);
	scene->TextComponents.Create(entity);
	scene->TextRenderers.Create(entity);
	mTextEntity = entity;

	mTransitions["scale"] = CreateRef<UITransitionFactory<ScaleTransitionModule>>();
	mTransitions["scale-bounce"] = CreateRef<UITransitionFactory<ScaleBounceTransitionModule>>();
	mTransitions["slide-right-bounce"] = CreateRef<UITransitionFactory<SlideRightBounceTransitionModule>>();
	mTransitions["slide-right"] = CreateRef<UITransitionFactory<SlideRightTransitionModule>>();
	mTransitions["slide-left"] = CreateRef<UITransitionFactory<SlideLeftTransitionModule>>();
}

SceneUI::~SceneUI()
{

}

void SceneUI::AddComponentRenderer(UIComponentType type, UIFuncRenderPtr funcPtr, const char* rendertype)
{
	s_RenderFuncvtable[(int)type] = { funcPtr, rendertype };
}

void SceneUI::CreateUIPanel(const std::string& panelName, const std::string& jsonFile)
{
	auto file = ZVFS::GetFile(jsonFile.c_str());
	nlohmann::json json = nlohmann::json::parse(file->Str());

	UIPanel panel;

	panel.Name = panelName;
	panel.Path = jsonFile;
	panel.PanelResolution = glm::ivec2(mScene->VirtualScreenSize);
	panel.PadDefault = GetJsonValue<std::string>(json, "pad-default", "");

	ParseTree(json["components"], NULL, panel);
	CalculateBBoxes(panel);
	
	for (auto& root : panel.Roots)
	{
		CalculateLayout(root.get());
	}

	if (mPanels.contains(panelName))
	{
		ReleasePanel(mPanels[panelName]);
	}

	mPanels[panelName] = panel;
}

void SceneUI::ReleasePanel(UIPanel& panel)
{
	std::queue<Ref<UIComponent>> componentQueue;

	for (auto& root : panel.Roots)
	{
		componentQueue.push(root);
	}

	while (!componentQueue.empty())
	{
		auto& component = componentQueue.front();
		componentQueue.pop();

		for (auto& state : component->uimg.states)
		{
			//if (state.second.embed)
			//	mScene->Resources.ReleaseImage(state.second.handle);
		}

		for (auto child : component->Components)
		{
			componentQueue.push(child);
		}
	}
}

void SceneUI::ShowUIPanel(const std::string& panelName)
{
	if (!mPanels.contains(panelName))
		return;
	auto& panel = mPanels[panelName];

	if (panel.Active)
		return;

	panel.Active = true;

	for (auto& focus : mFocusedPanels)
	{
		if (focus == panelName)
			return;
	}

	mFocusedPanels.push_back(panelName);

	static std::queue<UIComponent*> componentQueue;

	for (auto& root : panel.Roots)
	{
		componentQueue.push(root.get());
	}

	while (!componentQueue.empty())
	{
		auto& component = *componentQueue.front();
		componentQueue.pop();
		
		component.TransitionProgress = 0.0f;

		if (!component.TransitionModuleIn)
		{
			if (component.TransitionState != UIPanelTransitionState::IDLE)
			{
				component.TransitionState = UIPanelTransitionState::IDLE;
			}
		}
		else
		{
			component.TransitionState = UIPanelTransitionState::SHOWING;
			component.TransitionStartTimer = component.TransitionOffset;
		}

		for (auto& child : component.Components)
		{
			componentQueue.push(child.get());
		}
	}
}

void SceneUI::HideUIPanel(const std::string& panelName)
{
	if (!mPanels.contains(panelName))
		return;
	auto& panel = mPanels[panelName];

	if (!panel.Active)
		return;

	panel.Active = false;

	for (int i = 0; i < mFocusedPanels.size(); i++)
	{
		if (mFocusedPanels[i] == panelName)
		{
			mFocusedPanels.erase(mFocusedPanels.begin() + i);
			break;
		}
	}

	static std::queue<UIComponent*> componentQueue;

	for (auto& root : panel.Roots)
	{
		componentQueue.push(root.get());
	}

	while (!componentQueue.empty())
	{
		auto& component = *componentQueue.front();
		componentQueue.pop();

		if (!component.TransitionModuleOut)
			continue;

		if (component.TransitionState == UIPanelTransitionState::HIDING)
			return;

		component.TransitionState = UIPanelTransitionState::HIDING;
		component.TransitionProgress = component.TransitionLength;

		panel.Active = true;

		for (auto& child : component.Components)
		{
			componentQueue.push(child.get());
		}
	}
}

void SceneUI::Update()
{
	glm::vec2 MousePosition = mScene->VirtualMousePosition;// -(mScene->VirtualScreenSize * glm::vec2(-1, 1));
	MousePosition /= mScene->VirtualScreenSize;
	//MousePosition = MousePosition * 2.0f - 1.0f;

	std::array<ECS::edict_t, 16> cameras{};

	auto& sceneCameras = mScene->Cameras.GetEntities();

	for (auto entity : sceneCameras)
	{
		auto& camera = mScene->Cameras.Get(entity);
		if (camera.RenderLayer < 16)
		{
			cameras[camera.RenderLayer] = entity;
		}
	}

	static glm::vec2 lastMousePos = {};

	if (lastMousePos != MousePosition)
	{
		hoveredComponent = nullptr;
		lastMousePos = MousePosition;

		Stratum::Input::SetInputMode(Stratum::MouseInputMode::Normal);
		mIsMouseHidden = false;

		if (hoveredComponent && 
			(hoveredComponent->Type != UIComponentType::BUTTON &&
				hoveredComponent->Type != UIComponentType::CHECKBOX))
		{
			glm::vec2 pos = { hoveredComponent->GetPositionX(), hoveredComponent->GetPositionY() };

			AABB aabb = AABB(pos, hoveredComponent->BoundingBoxExtends);

			auto& camera = mScene->Cameras.Get(cameras[hoveredComponent->CameraLayer]);

			glm::vec2 mousePosition = camera.ScreenPointToWorld(glm::vec3(MousePosition, 0.0f));
			mousePosition -= (mScene->VirtualScreenSize * glm::vec2(-1, 1));

			if (!aabb.PointInside(mousePosition))
			{
				hoveredComponent = nullptr;
			}
		}

		for (auto& kv : mPanels)
		{
			auto& panel = kv.second;

			if (!panel.Active)
				continue;

			if (panel.PanelResolution != glm::ivec2(mScene->VirtualScreenSize))
			{
				CreateUIPanel(panel.Name, panel.Path);
				panel = mPanels[panel.Name];
				panel.Active = true;
			}

			static std::queue<UIComponent*> componentQueue;

			for (auto& root : panel.Roots)
			{
				componentQueue.push(root.get());

				while (!componentQueue.empty())
				{
					auto component = componentQueue.front();
					componentQueue.pop();

					glm::vec2 pos = { component->GetPositionX(), component->GetPositionY() };

					AABB aabb = AABB(pos, component->BoundingBoxExtends);

					auto& camera = mScene->Cameras.Get(cameras[component->CameraLayer]);
					glm::vec2 mousePosition = camera.ScreenPointToWorld(glm::vec3(MousePosition, 0.0f));
					mousePosition -= (mScene->VirtualScreenSize * glm::vec2(-1, 1));

					if (aabb.PointInside(mousePosition) && component->TransitionState == UIPanelTransitionState::IDLE)
					{
						if (!hoveredComponent || component->RenderLayer >= hoveredComponent->RenderLayer)
						{
							hoveredComponent = component;
						}
					}

					for (auto& child : component->Components)
					{
						componentQueue.push(child.get());
					}
				}
			}
		}
	}

	for (auto& kv : mPanels)
	{
		auto& panel = kv.second;

		static std::queue<UIComponent*> componentQueue;

		for (auto& root : panel.Roots)
		{
			componentQueue.push(root.get());

			while (!componentQueue.empty())
			{
				auto component = componentQueue.front();
				componentQueue.pop();

				if (component != hoveredComponent)
				{
					component->Hovered = false;
				}

				for (auto& child : component->Components)
				{
					componentQueue.push(child.get());
				}
			}
		}
	}

	bool usedController = false;

	static std::string lastFocus = "";
	static std::string focus = "";

	focus.clear();

	if (!mFocusedPanels.empty())
	{
		focus = mFocusedPanels.back();
	}

	if ((Stratum::Input::AnyGamepadDown() &&
		(!hoveredComponent || (hoveredComponent && 
			(hoveredComponent->Type != UIComponentType::BUTTON &&
			hoveredComponent->Type != UIComponentType::CHECKBOX)))) ||
		(focus != lastFocus && mIsMouseHidden))
	{
		hoveredComponent = nullptr;

		lastFocus = focus;

		if (mPanels.contains(focus))
		{
			auto& panel = mPanels[focus];
			if (!panel.PadDefault.empty() && panel.Active && !hoveredComponent)
			{
				hoveredComponent = FindObject(panel.Name, panel.PadDefault);
			}
		}
		else
		{
			for (auto& kv : mPanels)
			{
				auto& panel = kv.second;
				if (!panel.PadDefault.empty() && panel.Active && !hoveredComponent)
				{
					hoveredComponent = FindObject(panel.Name, panel.PadDefault);
				}
			}
		}

		if (Stratum::Input::AnyGamepadDown())
		{
			usedController = true;
		}
	}

	if (hoveredComponent)
	{
		if ((mPanels.contains(hoveredComponent->PanelName) && !mPanels[hoveredComponent->PanelName].Active) || hoveredComponent->TransitionState != UIPanelTransitionState::IDLE)
		{
			hoveredComponent = nullptr;
			return;
		}

		if (!hoveredComponent)
			return;

		if (!hoveredComponent->Hovered && !hoveredComponent->OnHover.empty())
		{
			AppUIEvent e;
			e.ElementName = hoveredComponent->Name;
			e.PanelName = hoveredComponent->PanelName;
			e.EventName = hoveredComponent->OnHover;
			EventBus::InvokeEvent<AppUIEvent>(e);
		}

		if (hoveredComponent->Type == UIComponentType::BUTTON && (Input::GetMouseButtonDown(0) || Stratum::Input::GetGamepadButtonDown(GamepadButton::A)))
		{
			AppUIEvent e;
			e.ElementName = hoveredComponent->Name;
			e.PanelName = hoveredComponent->PanelName;
			e.EventName = hoveredComponent->Button.OnClick;
			EventBus::InvokeEvent<AppUIEvent>(e);
		}

		if (hoveredComponent->Type == UIComponentType::CHECKBOX && (Input::GetMouseButtonDown(0) || Stratum::Input::GetGamepadButtonDown(GamepadButton::A)))
		{
			hoveredComponent->Checkbox.value = !hoveredComponent->Checkbox.value;
		}

		hoveredComponent->Hovered = true;

		std::array<std::string*, 4> next = { &hoveredComponent->PadUp, &hoveredComponent->PadDown, &hoveredComponent->PadLeft, &hoveredComponent->PadRight };
		std::array<GamepadButton, 4> buttons = { GamepadButton::DPAD_UP, GamepadButton::DPAD_DOWN, GamepadButton::DPAD_LEFT, GamepadButton::DPAD_RIGHT };

		for (int i = 0; i < 4; i++)
		{
			if (Stratum::Input::GetGamepadButtonDown(buttons[i]))
			{
				usedController = true;
				if (!next[i]->empty())
				{
					auto nextComp = FindObject(hoveredComponent->PanelName, *next[i]);
					if (nextComp)
					{
						hoveredComponent = nextComp;
						break;
					}
				}
			}
		}
	}

	if (usedController && HideMouseWithController)
	{
		Stratum::Input::SetInputMode(Stratum::MouseInputMode::Hidden);
		mIsMouseHidden = true;
	}
}

void SceneUI::PreRender(Render::CopyCommandBuffer* pCmd)
{
	
}

void SceneUI::Render(RenderQueue2D* ppRenderQueues)
{
	static std::queue<UIComponent*> componentQueue;

	for (auto& kv : mPanels)
	{
		auto& panel = kv.second;

		if (!panel.Active)
			continue;

		for (auto& root : panel.Roots)
		{
			componentQueue.push(root.get());
		}

		if (TransitionFinished(panel))
		{
			panel.Active = false;
		}
	}

	while (!componentQueue.empty())
	{
		auto& component = *componentQueue.front();
		componentQueue.pop();

		if (component.CameraLayer >= 16)
		{
			continue;
		}

		float x = component.GetPositionX();
		float y = component.GetPositionY();
		float p = 0.0f;

		if (component.TransitionStartTimer > 0.0f)
		{
			component.TransitionStartTimer -= gpGlobals->deltaTime;
		}
		else
		{
			if (component.TransitionState == UIPanelTransitionState::SHOWING)
			{
				component.TransitionProgress += Stratum::gpGlobals->deltaTime;
				if (component.TransitionProgress >= component.TransitionLength)
				{
					component.TransitionProgress = 0.0f;
					component.TransitionState = UIPanelTransitionState::IDLE;
				}
				p = component.TransitionProgress / component.TransitionLength;
			}

			if (component.TransitionState == UIPanelTransitionState::HIDING)
			{
				if (component.TransitionProgress > 0.0f)
				{
					component.TransitionProgress -= Stratum::gpGlobals->deltaTime;
				}
				p = component.TransitionProgress / component.TransitionLength;
			}
		}

		p = glm::clamp(p, 0.0f, 1.0f);

		glm::mat4 transform = glm::mat4(1.0f);
		glm::vec4 color = glm::vec4(1.0f);

		if (component.TransitionState != UIPanelTransitionState::IDLE)
		{
			auto module = component.TransitionState == UIPanelTransitionState::SHOWING ? component.TransitionModuleIn : component.TransitionModuleOut;
			if (module)
			{
				transform *= module->GetMatrix(&component, p);
				color = module->GetColor(&component, p);
				component.TransitionColor = color;
			}
		}

		transform = glm::translate(transform, glm::vec3(mScene->VirtualScreenSize * glm::vec2(-1.0f, 1.0f), 0.0f));
		transform = glm::translate(transform, glm::vec3(x, y, 0.0f));

		auto t_render = &s_RenderFuncvtable[(int)component.Type];

		if (component.Type == UIComponentType::RECT ||
			(t_render->func && strncmp(t_render->rendertype, "rect", 4) == 0))
		{
			Render2DInstance instance{};

			instance.batch.center = glm::vec2(-1.0f, 1.0f);
			instance.batch.rect = { glm::ivec2(0), glm::ivec2(component.Width, component.Height) };
			instance.batch.RenderSize = instance.batch.rect.size;
			instance.batch.transform = transform;
			instance.zIndex = component.RenderLayer;
			instance.batch.color = component.BgColor;
			instance.batch.useNearestFilter = false;
			instance.batch.texture = component.Background;
			instance.batch.scaleWithRenderSize = false;

			if (t_render->func)
			{
				t_render->func(&component, &instance);
			}

			if (instance.batch.texture != -1)
			{
				instance.batch.rect.size = mScene->Resources.GetImageHandle(instance.batch.texture)->GetSize();
			}

			instance.batch.color *= color;

			ppRenderQueues[component.CameraLayer].Push(instance);
		}

		if (component.Type == UIComponentType::LABEL)
		{
			Render2DInstance instance{};

			instance.kind = Render2DInstanceKind::TEXT;

			instance.text.textEntity = mTextEntity;
			instance.text.userData = &component;
			instance.batch.transform = transform;
			instance.zIndex = component.RenderLayer;

			ppRenderQueues[component.CameraLayer].Push(instance);
		}

		for (auto& child : component.Components)
		{
			componentQueue.push(child.get());
		}
	}
}

void SceneUI::OnTextRender(Render2DInstance* pInstance)
{
	auto component = reinterpret_cast<UIComponent*>(pInstance->text.userData);

	if (!component || pInstance->text.textEntity != mTextEntity)
	{
		mScene->TextComponents.Get(mTextEntity).FontSize = 0.0f;
		return;
	}
	if (component->Type != UIComponentType::LABEL)
		return;

	auto& target = mScene->TextComponents.Get(mTextEntity).Text;

	if (target.capacity() < component->Label.Text.size())
		target.reserve(component->Label.Text.size() + 1);

	target.clear();
	for (int i = 0; i < component->Label.Text.size(); i++)
	{
		target.push_back(component->Label.Text[i]);
	}
	
	mScene->TextComponents.Get(mTextEntity).FontSize = component->FontSize;
	mScene->TextComponents.Get(mTextEntity).Font = component->Font;
	mScene->TextRenderers.Get(mTextEntity).Alignment = component->Label.TextAlignment;
	mScene->TextRenderers.Get(mTextEntity).Color = component->FgColor;
	mScene->TextRenderers.Get(mTextEntity).Color *= component->TransitionColor;
	mScene->Transforms.Get(mTextEntity).ModelMatrix = pInstance->batch.transform;
}

UIComponent* SceneUI::FindObject(const std::string& panelName, const std::string& name)
{
	static std::queue<UIComponent*> componentQueue;

	if (mPanels.contains(panelName))
	{
		for (auto& root : mPanels[panelName].Roots)
		{
			componentQueue.push(root.get());

			while (!componentQueue.empty())
			{
				auto& component = componentQueue.front();
				componentQueue.pop();

				if (component->Name.compare(name) == 0)
				{
					while(!componentQueue.empty())
						componentQueue.pop();

					return component;
				}

				for (auto& child : component->Components)
				{
					componentQueue.push(child.get());
				}
			}
		}
	}

	return nullptr;
}

bool SceneUI::IsMouseHidden()
{
	return mIsMouseHidden;
}

void SceneUI::CalculateBBoxes(UIPanel& panel)
{
	std::queue<Ref<UIComponent>> componentQueue;

	for (auto& root : panel.Roots)
	{
		componentQueue.push(root);
	}

	// Bounding boxes

	while (!componentQueue.empty())
	{
		auto component = componentQueue.front();
		componentQueue.pop();

		component->BoundingBoxExtends = ComputeComponentBBox(component.get());
		component->LayoutBoundingBoxExtends = component->BoundingBoxExtends +
			glm::vec2(component->PaddingRight, component->PaddingBottom);

		for (auto child : component->Components)
		{
			componentQueue.push(child);
		}
	}

}

void SceneUI::CalculateLayout(UIComponent* component)
{
	if (!component->Parent)
	{
		component->Position.x += component->TransformX;
		component->Position.y -= component->TransformY;

		component->Position.x += component->PaddingLeft;
		component->Position.y -= component->PaddingTop;

		if (component->Anchor.compare("right") == 0)
		{
			component->Position.x += mScene->VirtualScreenSize.x * 2.0f;
		}
	}

	glm::vec2 pointer = {};

	for (auto comp : component->Components)
	{

		auto& component = *comp;

		glm::vec2 ptrBefore = pointer;

		if (component.Anchor.compare("right") == 0)
		{
			pointer.x = mScene->VirtualScreenSize.x * 2.0f;
		}

		pointer.x += component.PaddingLeft;
		pointer.y -= component.PaddingTop;

		pointer.x += component.TransformX;
		pointer.y -= component.TransformY;

		component.Position = pointer;

		if (component.Type == UIComponentType::LABEL)
		{
			component.Position.y -= component.BoundingBoxExtends.y;
			//component.LayoutBoundingBoxExtends.y += component.BoundingBoxExtends.y;
		}

		if (component.NextElement == UINextElement::RIGHT)
		{
			pointer.x += component.LayoutBoundingBoxExtends.x + component.PaddingRight;
		}
		else if (component.NextElement == UINextElement::BOTTOM || component.NextElement == UINextElement::INVALID)
		{
			pointer.x = 0.0f;
			pointer.y -= component.LayoutBoundingBoxExtends.y + component.PaddingBottom;
		}

		if (component.IsBackground)
			pointer = ptrBefore;

		CalculateLayout(comp.get());

	}
}

bool SceneUI::TransitionFinished(UIPanel& panel)
{
	static std::queue<UIComponent*> componentQueue;

	for (auto& root : panel.Roots)
	{
		componentQueue.push(root.get());
	}

	// Bounding boxes

	while (!componentQueue.empty())
	{
		auto component = componentQueue.front();
		componentQueue.pop();

		if (component->TransitionState != UIPanelTransitionState::HIDING)
		{
			while (!componentQueue.empty())
				componentQueue.pop();
			return false;
		}

		if (component->TransitionState == UIPanelTransitionState::HIDING && component->TransitionProgress > 0.0f)
		{
			while (!componentQueue.empty())
				componentQueue.pop();
			return false;
		}

		for (auto& child : component->Components)
		{
			componentQueue.push(child.get());
		}
	}

	return true;
}

float SceneUI::GetComponentWidth(UIComponent* component)
{
	if (component)
		return component->Width;
	return mScene->VirtualScreenSize.x * 2.0f;
}

float SceneUI::GetComponentHeight(UIComponent* component)
{
	if (component)
		return component->Height;
	return mScene->VirtualScreenSize.y * 2.0f;
}

glm::vec2 SceneUI::ComputeComponentBBox(UIComponent* component)
{
	if (component->Type == UIComponentType::LABEL)
	{
		TextBatcher textRenderer(NULL); // Dummy batcher
		TextBatcherParameters parameters;
		parameters.maxWidth = 800.0f;
		parameters.wrapText = false;
		parameters.fontSize = component->FontSize;
		parameters.lineHeight = 1.0f;
		parameters.font = mScene->FontRegistry.GetFont("Roboto");
		textRenderer.SetParameters(parameters);

		glm::vec2 size = textRenderer.GetStringSize(component->Label.Text);
		return size;
	}
	if (component->Type == UIComponentType::BUTTON || component->Type == UIComponentType::RECT || component->Type == UIComponentType::CHECKBOX)
	{
		return { component->Width, component->Height };
	}

	return {};
}

void SceneUI::ParseTree(nlohmann::json& json, UIComponent* Parent, UIPanel& panel)
{
	glm::vec2 startPosition = { 0.0f, 0.0f };
	for (auto& comp : json)
	{
		Ref<UIComponent> component = CreateRef<UIComponent>();

		float parentWidth = GetComponentWidth(Parent);
		float parentHeight = GetComponentHeight(Parent);

		component->Type = UIComponentType::INVALID;
		component->Parent = Parent;
		component->PanelName = panel.Name;
		component->IsBackground = GetJsonValue<bool>(comp, "is-background", false);
		component->OnHover = GetJsonValue<std::string>(comp, "onHover", "");
		component->Anchor = GetJsonValue<std::string>(comp, "anchor", "");
		component->FontSize = GetJsonValue<float>(comp, "font-size", component->GetRootEM());
		component->Width = ParseUnits(GetJsonValue<std::string>(comp, "width", ""), parentWidth, component->GetRootEM(), component->FontSize);
		component->Height = ParseUnits(GetJsonValue<std::string>(comp, "height", ""), parentHeight, component->GetRootEM(), component->FontSize);
		component->PaddingTop = ParseUnits(GetJsonValue<std::string>(comp, "padding-top", ""), parentHeight, component->GetRootEM(), component->FontSize);
		component->PaddingBottom = ParseUnits(GetJsonValue<std::string>(comp, "padding-bottom", ""), parentHeight, component->GetRootEM(), component->FontSize);
		component->PaddingLeft = ParseUnits(GetJsonValue<std::string>(comp, "padding-left", ""), parentWidth, component->GetRootEM(), component->FontSize);
		component->PaddingRight = ParseUnits(GetJsonValue<std::string>(comp, "padding-right", ""), parentWidth, component->GetRootEM(), component->FontSize);
		component->TransformX = ParseUnits(GetJsonValue<std::string>(comp, "transform-x", ""), parentWidth, component->GetRootEM(), component->FontSize);
		component->TransformY = ParseUnits(GetJsonValue<std::string>(comp, "transform-y", ""), parentHeight, component->GetRootEM(), component->FontSize);
		component->PadUp = GetJsonValue<std::string>(comp, "up", "");
		component->PadDown = GetJsonValue<std::string>(comp, "down", "");
		component->PadLeft = GetJsonValue<std::string>(comp, "left", "");
		component->PadRight = GetJsonValue<std::string>(comp, "right", "");
		component->Name = GetJsonValue<std::string>(comp, "name", "");

		if (comp.contains("uimg"))
		{
			component->uimg = LoadUIMG(comp["uimg"]);
		}

		if (Parent)
		{
			component->TransitionModuleIn = Parent->TransitionModuleIn;
			component->TransitionModuleOut = Parent->TransitionModuleOut;
			component->TransitionLength = Parent->TransitionLength;
			component->Font = Parent->Font;
		}

		component->Font = GetJsonValue<std::string>(comp, "font", component->Font);
		component->TransitionLength = GetJsonValue<float>(comp, "transition-len", component->TransitionLength);
		component->TransitionOffset = GetJsonValue<float>(comp, "transition-offset", 0.0f);

		if (comp.contains("background-color"))
		{
			ParseVector(glm::value_ptr(component->BgColor), 4, comp["background-color"]);
		}

		if (comp.contains("foreground-color"))
		{
			ParseVector(glm::value_ptr(component->FgColor), 4, comp["foreground-color"]);
		}

		if (comp.contains("unhovered-color"))
		{
			ParseVector(glm::value_ptr(component->Button.UnhoveredColor), 4, comp["unhovered-color"]);
		}
		if (comp.contains("hovered-color"))
		{
			ParseVector(glm::value_ptr(component->Button.HoveredColor), 4, comp["hovered-color"]);
		}

		if (comp["type"] == "rect")
		{
			component->Type = UIComponentType::RECT;
		}
		else if (comp["type"] == "label")
		{
			component->Type = UIComponentType::LABEL;
			component->Label.Text = Utils::ToWideString(GetJsonValue<std::string>(comp, "text", ""));
			component->Label.TextAlignment = ParseUnits(GetJsonValue<std::string>(comp, "text-align", ""), 1.0f, 1.0f, 1.0f);
		}
		else if (comp["type"] == "button")
		{
			component->Type = UIComponentType::BUTTON;
			component->Button.OnClick = GetJsonValue<std::string>(comp, "onClick", "");
		}
		else if (comp["type"] == "checkbox") 
		{
			component->Type = UIComponentType::CHECKBOX;
			component->Checkbox.value = false;
		}

		if (comp.contains("next-element"))
		{
			if (comp["next-element"] == "right")
			{
				component->NextElement = UINextElement::RIGHT;
			}
			else if (comp["next-element"] == "bottom")
			{
				component->NextElement = UINextElement::BOTTOM;
			}
		}

		auto transitionNameIn = GetJsonValue<std::string>(comp, "transition-in", "");
		auto transitionNameOut = GetJsonValue<std::string>(comp, "transition-out", "");

		if (mTransitions.contains(transitionNameIn))
		{
			component->TransitionModuleIn = mTransitions[transitionNameIn]->pModule;
		}

		if (mTransitions.contains(transitionNameOut))
		{
			component->TransitionModuleOut = mTransitions[transitionNameOut]->pModule;
		}

		if (comp.contains("background"))
		{
			component->Background = mScene->Resources.LoadTextureImage(comp["background"]);

			if (component->Height == -64.0f)
			{
				auto size = mScene->Resources.GetImageHandle(component->Background)->GetSize();
				float scale = component->Width / size.x;
				component->Height = size.y * scale;
			}
			if (component->Width == -64.0f)
			{
				auto size = mScene->Resources.GetImageHandle(component->Background)->GetSize();
				float scale = component->Height / size.y;
				component->Width = size.x * scale;
			}
		}

		uint8_t cameraLayer = Parent ? Parent->CameraLayer : 0;
		uint32_t renderLayer = Parent ? Parent->RenderLayer : 0;

		component->CameraLayer = GetJsonValue<uint8_t>(comp, "camera-layer", cameraLayer);
		component->RenderLayer = GetJsonValue<uint32_t>(comp, "render-layer", renderLayer + 1);

		if (comp.contains("components"))
		{
			ParseTree(comp["components"], component.get(), panel);
		}

		if (Parent)
		{
			Parent->Components.push_back(component);
		}
		else
		{
			panel.Roots.push_back(component);
		}
	}
}

float SceneUI::ParseUnits(const std::string& str, float relative, float rem, float em)
{
	if (str.compare("auto") == 0)
	{
		return -64.0f;
	}
	if (str.ends_with("%"))
	{
		float f = std::atof(str.substr(0, str.size() - 1).c_str());
		return (f / 100.0f) * relative;
	}

	if (str.ends_with("rem"))
	{
		float f = std::atof(str.substr(0, str.size() - 3).c_str());
		return f * rem;
	}

	if (str.ends_with("em"))
	{
		float f = std::atof(str.substr(0, str.size() - 2).c_str());
		return f * em;
	}

	if (str.ends_with("px"))
	{
		float f = std::atof(str.substr(0, str.size() - 2).c_str());
		return f;
	}

	return 0.0f;
}

void SceneUI::ParseVector(float* dest, int nbComps, nlohmann::json& json)
{
	if (!json.is_array())
		return;
	for (int i = 0; i < nbComps && i < json.size(); i++)
	{
		dest[i] = json[i].get<float>();
	}
}

t_userimage SceneUI::LoadUIMG(const std::string& path)
{
	auto file = ZVFS::GetFile(path.c_str());
	nlohmann::json json = nlohmann::json::parse(file->Str());
	t_userimage container{};
	for (auto& state : json["states"].items())
	{
		auto& substate = state.value();
		t_userimage::t_state uimg{};
		if (substate.contains("embed"))
		{
			std::string bf = substate["embed"];
			bf = base64::from_base64(bf);
			int width;
			int height;
			int nrChannels;
			auto buff = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(bf.data()),
				static_cast<int>(bf.size()),
				&width,
				&height,
				&nrChannels,
				0);
			Render::ImageDescription imageDesc{};

			switch (nrChannels)
			{
			case 1:
				imageDesc.Format = Render::ImageFormat::R8_UNORM;
				break;
			case 2:
				imageDesc.Format = Render::ImageFormat::RG8_UNORM;
				break;
			default:
				imageDesc.Format = Render::ImageFormat::RGBA8_UNORM;
				break;
			}

			imageDesc.Width = width;
			imageDesc.Height = height;
			imageDesc.Immutable = true;
			imageDesc.MipLevels = 1;
			imageDesc.DefaultData = { { buff, (uint32_t)(width * nrChannels) } };

			uimg.handle = mScene->Resources.CreateTextureImage(imageDesc);
			uimg.texture = uimg.handle;
			uimg.embed = true;
		}
		else
		{
			std::string loc = substate["src"];
			uimg.texture = mScene->Resources.LoadTextureImage(loc);
		}

		uimg.render_width = GetJsonValue(substate, "width", 0);
		uimg.render_height = GetJsonValue(substate, "height", 0);
		uimg.offset_x = GetJsonValue(substate, "offset-x", 0);
		uimg.offset_y = GetJsonValue(substate, "offset-y", 0);
		container.states[state.key()] = uimg;
	}
	return container;
}