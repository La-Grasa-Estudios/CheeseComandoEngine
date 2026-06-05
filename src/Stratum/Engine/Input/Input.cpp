#include "Input.h"
#include "InputLayer.h"
#include "Core/Logger.h"
#include "Event/EventBus.h"

#include <vcruntime_string.h>
#include <SDL3/SDL.h>

using namespace ENGINE_NAMESPACE;

bool g_IsMouseGrabbed = false;

// TO DO: Add support for input layers
// Need that so UI can capture clicks and key presses and don't send it through the actual game logic
// Maybe rewrite the entire thing? (Originally GLFW then SDL ass ugly port)
// This thing comes from 2019
// There is a reason the engine is called Stratum lol

std::unordered_map<SDL_JoystickID, SDL_Gamepad*> g_Gamepads;
SDL_JoystickID gLastGamepadInput = 0;

bool BaseInputLayer::SetKey(int key, bool press)
{
	Input::m_Keys[key] = press;
	return true;
}

bool BaseInputLayer::SetMouse(int click, bool press)
{
	int button = click;
	if (button == 1) button = 0;
	if (button == 3) button = 1;
	Input::m_Mouse[button] = press;
	return true;
}

bool BaseInputLayer::SetGamepad(int button, bool press)
{
	Input::m_GamePadButtons[button] = press;
	return true;
}

bool BaseInputLayer::SetGamepadAxis(int axis, int16_t value)
{
	Input::m_GamepadAxis[axis] = value;
	return true;
}

void Input::Init(SDL_Window* window)
{
	m_Window = window;
	s_MousePosition = GetMousePosition();
	memset(m_Keys, 0, sizeof(m_Keys));
	memset(m_LastKeys, 0, sizeof(m_LastKeys));
	memset(m_GamePadButtons, 0, sizeof(m_GamePadButtons));
	memset(m_GamepadAxis, 0, sizeof(m_GamepadAxis));

	SDL_LockJoysticks();
	SDL_AddGamepadMappingsFromFile("Engine/gamecontrollerdb.txt");
	SDL_UnlockJoysticks();

	EventBus::RegisterListener<ApplicationSDLEvent>([&](const ApplicationSDLEvent& event) {
		SDL_Event& e = *reinterpret_cast<SDL_Event*>(event.pEventData);
		
		if (e.type == SDL_EVENT_GAMEPAD_ADDED)
		{
			m_GamepadCount++;
			g_Gamepads[e.gdevice.which] = SDL_OpenGamepad(e.gdevice.which);
			gLastGamepadInput = e.gdevice.which;
			Z_INFO("Gamepad found! id: {}-{}", e.gdevice.which, SDL_GetGamepadName(g_Gamepads[e.gdevice.which]))
		}

		if (e.type == SDL_EVENT_GAMEPAD_REMOVED)
		{
			m_GamepadCount--;
			SDL_CloseGamepad(g_Gamepads[e.gdevice.which]);
			g_Gamepads.erase(e.gdevice.which);
		}

		switch (e.type)
		{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			SetKey(e.key.keysym.scancode, e.key.state);
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			SetGamepad(e.gbutton.button, e.gbutton.state);
			gLastGamepadInput = e.gbutton.which;
			break;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			SetGamepadAxis(e.gaxis.axis, e.gaxis.value);
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:

			SetMouse(e.button.button, e.button.state);
			//Input::SetMousePos(glm::vec2((float)xpos, (float)ypos));
			break;
		case SDL_EVENT_MOUSE_MOTION:
			Input::s_ThreadedMousePos = glm::vec2(e.motion.x, e.motion.y);
			break;
		case SDL_EVENT_MOUSE_WHEEL:
				m_ScrollDelta += e.wheel.y;
			break;
		default:
			break;
		}

		}, EF_NONE);

	PushInputLayer(new BaseInputLayer()); // Push default input layer
}

void Input::SetKey(int key, bool press)
{
	for (int i = 0; i < sLayers.size(); i++)
	{
		auto layer = sLayers[i];
		if (layer->SetKey(key, press)) break;
	}
}

void Input::SetMouse(int click, bool press)
{
	for (int i = 0; i < sLayers.size(); i++)
	{
		auto layer = sLayers[i];
		if (layer->SetMouse(click, press)) break;
	}
}

void Input::SetMousePos(glm::vec2 pos)
{
	SDL_WarpMouseInWindow(m_Window, pos.x, pos.y);
}

void Input::SetGamepad(int button, bool press)
{
	for (int i = 0; i < sLayers.size(); i++)
	{
		auto layer = sLayers[i];
		if (layer->SetGamepad(button, press)) break;
	}
}

void Input::SetGamepadAxis(int axis, int16_t value)
{
	for (int i = 0; i < sLayers.size(); i++)
	{
		auto layer = sLayers[i];
		if (layer->SetGamepadAxis(axis, value)) break;
	}
}

void Input::SetGamepadRumble(float left_intensity, float right_intensity, uint32_t duration)
{
	for (auto gpad : g_Gamepads)
	{
		if (SDL_GamepadHasRumble(gpad.second))
		{
			SDL_RumbleGamepad(gpad.second, (uint16_t)(glm::clamp(left_intensity, 0.0f, 1.0f) * 0xFFFF), (uint16_t)(glm::clamp(right_intensity, 0.0f, 1.0f) * 0xFFFF), duration);
		}
	}
}

int Input::GetScrollDelta()
{
	return m_ScrollDelta;
}

void Input::SetInputMode(MouseInputMode mode)
{
	int input = 0;
	switch (mode)
	{
	case MouseInputMode::Normal:
		input = 0;
		g_IsMouseGrabbed = false;
		break;
	case MouseInputMode::Hidden:
		input = 1;
		g_IsMouseGrabbed = false;
		break;
	case MouseInputMode::Disabled:
		input = 1;
		g_IsMouseGrabbed = true;
		break;
	default:
		break;
	}

	SDL_SetRelativeMouseMode((SDL_bool)input);
}

bool Input::GetKeyDown(KeyCode keyCode)
{
	return m_Keys[(int)keyCode] && !m_LastKeys[(int)keyCode];
}

bool Input::GetKey(KeyCode keyCode)
{
	return m_Keys[(int)keyCode];
}

bool Input::GetMouseButton(int button)
{
	return m_Mouse[button];
}

bool Input::GetMouseButtonDown(int button)
{
	return m_Mouse[button] && !m_LastMouse[button];;
}

bool Input::GetGamepadButton(GamepadButton button)
{
	return m_GamePadButtons[(int32_t)button];
}

bool Input::GetGamepadButtonDown(GamepadButton button)
{
	return m_GamePadButtons[(int32_t)button] && !m_LastGamePadButtons[(int32_t)button];
}

float Input::GetGamepadAxis(GamepadAxis axis)
{
	float a = m_GamepadAxis[(int32_t)axis] / (float)(SDL_JOYSTICK_AXIS_MAX);
	bool s = a < 0.0f;
	a = glm::max(glm::abs(a) - 0.1f, 0.0f) / 0.9f;
	return a * (s ? -1.0f : 1.0f);
}

int32_t Input::GetGamepadType()
{
	if (gLastGamepadInput)
	{
		if (g_Gamepads.contains(gLastGamepadInput))
		{
			return SDL_GetGamepadType(g_Gamepads[gLastGamepadInput]);
		}
	}

	return SDL_GAMEPAD_TYPE_UNKNOWN;
}

void Input::BindAlias(const char* alias, KeyCode keyCode)
{
	auto ptr = GetInputAlias(alias);
	ptr->keyCodes.insert(keyCode);
}

void Input::BindAlias(const char* alias, MouseButton button)
{
	auto ptr = GetInputAlias(alias);
	ptr->mouseButtons.insert(button);
}

void Input::BindAlias(const char* alias, GamepadButton gamepadButton)
{
	auto ptr = GetInputAlias(alias);
	ptr->gamepadButtons.insert((int32_t)gamepadButton);
}

void Input::BindAxisToAlias(const char* alias, GamepadAxis axis, float activation)
{
	auto ptr = GetInputAlias(alias);
	ptr->gpadAxis.insert((int32_t)axis);
}

bool Input::GetInput(const char* alias)
{
	auto ptr = GetInputAlias(alias);

	for (auto keycode : ptr->keyCodes)
	{
		if (GetKey(keycode))
			return true;
	}
	for (auto button : ptr->mouseButtons)
	{
		if (GetMouseButton(static_cast<int>(button)))
			return true;
	}
	for (auto gpad : ptr->gamepadButtons)
	{
		if (GetGamepadButton((GamepadButton)gpad))
			return true;
	}
	for (auto gpad : ptr->gpadAxis)
	{
		if (GetGamepadAxis((GamepadAxis)gpad) > 0.5f)
			return true;
	}

	return false;
}

bool Input::GetInputDown(const char* alias)
{
	auto ptr = GetInputAlias(alias);

	for (auto keycode : ptr->keyCodes)
	{
		if (GetKeyDown(keycode))
			return true;
	}
	for (auto button : ptr->mouseButtons)
	{
		if (GetMouseButtonDown(static_cast<int>(button)))
			return true;
	}
	for (auto gpad : ptr->gamepadButtons)
	{
		if (GetGamepadButtonDown((GamepadButton)gpad))
			return true;
	}
	for (auto gpad : ptr->gpadAxis)
	{
		if (GetGamepadAxis((GamepadAxis)gpad) > 0.5f)
		{
			if (ptr->active)
			{
				return false;
			}
			ptr->active = true;
			return true;
		}
	}

	ptr->active = false;

	return false;
}

bool Input::AnyKeyDown()
{
	return m_AnyKeyDown;
}

bool Input::AnyGamepadDown()
{
	return m_AnyGamepadDown;
}

bool Input::HasGamepadConnected()
{
	return m_GamepadCount > 0;
}

glm::vec2 Input::GetMousePosition()
{
	return s_ThreadedMousePos;
}

glm::vec2 Input::GetMouseSpeed()
{
	return s_MouseSpeed;
}

void Input::PushInputLayer(InputLayer_Interface* inputLayer)
{
	sLayers.push_back(inputLayer);
}

void Input::PopInputLayer()
{
	delete sLayers.back();
	sLayers.pop_back();
}

void Input::Update()
{
	m_ScrollDelta = 0;

	memcpy(m_LastKeys, m_Keys, sizeof(m_Keys));
	//memcpy(m_Keys, m_NextKeys, sizeof(m_Keys));

	memcpy(m_LastMouse, m_Mouse, sizeof(m_Mouse));
	//memcpy(m_Mouse, m_NextMouse, sizeof(m_Mouse));

	memcpy(m_LastGamePadButtons, m_GamePadButtons, sizeof(m_GamePadButtons));

	for (int i = 0; i < sLayers.size(); i++)
	{
		sLayers[i]->Update();
	}

	m_AnyKeyDown = false;
	m_AnyGamepadDown = false;

	for (int i = 0; i < 1000 && !m_AnyKeyDown; i++)
	{
		m_AnyKeyDown |= m_Keys[i];
		m_AnyKeyDown |= m_LastKeys[i];
		
	}

	for (int i = 0; i < MAX_BUTTONS && !m_AnyKeyDown; i++)
	{
		m_AnyKeyDown |= m_GamePadButtons[i];
		m_AnyGamepadDown |= m_GamePadButtons[i];
	}

	glm::vec2 mousePos = GetMousePosition();

	s_MouseSpeed = mousePos - s_MousePosition;

	s_MousePosition = mousePos;

	if (g_IsMouseGrabbed) {
		int w, h;

		SDL_GetWindowSizeInPixels(m_Window, &w, &h);

		SetMousePos(glm::vec2(w / 2, h / 2));
		s_MousePosition = glm::vec2(w / 2, h / 2);
	}

	float x = 0.0f;
	float y = 0.0f;
	SDL_GetMouseState(&x, &y);
	s_ThreadedMousePos = glm::vec2(x, y);

	SDL_UpdateGamepads();

}

Input::InputAlias* Input::GetInputAlias(const char* name)
{
	if (!sAliases.contains(name))
	{
		sAliases[name] = {};
	}
	return &sAliases[name];
}
