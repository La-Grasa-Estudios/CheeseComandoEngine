#include "Input.h"
#include "InputLayer.h"
#include "Core/Logger.h"
#include "Event/EventHandler.h"

#include <vcruntime_string.h>
#include <SDL3/SDL.h>

using namespace ENGINE_NAMESPACE;

bool g_IsMouseGrabbed = false;

// TO DO: Add support for input layers
// Need that so UI can capture clicks and key presses and don't send it through the actual game logic
// Maybe rewrite the entire thing? (Originally GLFW then SDL ass ugly port)
// This thing comes from 2019
// There is a reason the engine is called Stratum lol

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
	if (press)
	{
		Z_INFO("Gpad button {} last {}", Input::m_GamePadButtons[button], Input::m_LastGamePadButtons[button]);
	}
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

	EventHandler::RegisterListener([&](void*, void**, uint32_t) {

		m_GamepadCount += 1;

		}, EventHandler::GetEventID("gamepad_connect"));
	EventHandler::RegisterListener([&](void*, void**, uint32_t) {

		m_GamepadCount -= 1;

		}, EventHandler::GetEventID("gamepad_remove"));

	PushInputLayer(new BaseInputLayer()); // Push default input layer
}

void Input::SetKey(int key, bool press)
{
	for (int i = sLayers.size() - 1; i >= 0; i--)
	{
		auto layer = sLayers[i];
		if (layer->SetKey(key, press)) break;
	}
}

void Input::SetMouse(int click, bool press)
{
	for (int i = sLayers.size() - 1; i >= 0; i--)
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
	for (int i = sLayers.size() - 1; i >= 0; i--)
	{
		auto layer = sLayers[i];
		if (layer->SetGamepad(button, press)) break;
	}
}

void Input::SetGamepadAxis(int axis, int16_t value)
{
	for (int i = sLayers.size() - 1; i >= 0; i--)
	{
		auto layer = sLayers[i];
		if (layer->SetGamepadAxis(axis, value)) break;
	}
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

bool Input::GetMouseButttonDown(int button)
{
	return m_Mouse[button] && !m_LastMouse[button];;
}

bool Input::GetGamepadButton(int button)
{
	return m_GamePadButtons[button];
}

bool Input::GetGamepadButtonDown(int button)
{
	return m_GamePadButtons[button] && !m_LastGamePadButtons[button];
}

float Input::GetGamepadAxis(int axis)
{
	float a = m_GamepadAxis[axis] / (float)(INT16_MAX);
	bool s = a < 0.0f;
	a = glm::max(glm::abs(a) - 0.1f, 0.0f) / 0.9f;
	return a * (s ? -1.0f : 1.0f);
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
	memcpy(m_LastKeys, m_Keys, sizeof(m_Keys));
	//memcpy(m_Keys, m_NextKeys, sizeof(m_Keys));

	memcpy(m_LastMouse, m_Mouse, sizeof(m_Mouse));
	//memcpy(m_Mouse, m_NextMouse, sizeof(m_Mouse));

	memcpy(m_LastGamePadButtons, m_GamePadButtons, sizeof(m_GamePadButtons));

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
