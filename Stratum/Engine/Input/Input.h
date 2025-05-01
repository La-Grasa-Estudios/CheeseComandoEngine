#pragma once

#define MAX_KEYS 1000
#define MAX_BUTTONS 64

#include "znmsp.h"
#include <glm/glm.hpp>

#include "KeyCode.h"

struct SDL_Window;

BEGIN_ENGINE

enum class MouseInputMode {
	Normal,
	Hidden,
	Disabled,
};

class Input {

	static inline bool m_LastKeys[MAX_KEYS];
	static inline bool m_Keys[MAX_KEYS];
	static inline bool m_NextKeys[MAX_KEYS];

	static inline bool m_GamePadButtons[MAX_BUTTONS];
	static inline bool m_LastGamePadButtons[MAX_BUTTONS];

	static inline int16_t m_GamepadAxis[MAX_BUTTONS];
	
	static inline bool m_LastMouse[32];
	static inline bool m_Mouse[32];
	static inline bool m_NextMouse[32];

	static inline bool m_AnyKeyDown;
	static inline bool m_AnyGamepadDown;

	static inline uint8_t m_GamepadCount = 0;

	static inline glm::vec2 m_Pos;

	static inline SDL_Window* m_Window;

public:

	inline static glm::vec2 s_MousePosition = {};
	inline static glm::vec2 s_MouseSpeed = {};
	inline static glm::vec2 s_ThreadedMousePos = {};

	static void Init(SDL_Window* window);

	static void SetKey(int key, bool press);
	static void SetMouse(int click, bool press);
	static void SetMousePos(glm::vec2 pos);
	static void SetGamepad(int button, bool press);
	static void SetGamepadAxis(int axis, int16_t value);

	static void SetInputMode(MouseInputMode mode);

	static bool GetKeyDown(KeyCode keyCode);
	static bool GetKey(KeyCode keyCode);

	static bool GetMouseButton(int button);
	static bool GetMouseButttonDown(int button);

	static bool GetGamepadButton(int button);
	static bool GetGamepadButtonDown(int button);

	static float GetGamepadAxis(int axis);

	static bool AnyKeyDown();
	static bool AnyGamepadDown();
	static bool HasGamepadConnected();

	static glm::vec2 GetMousePosition();
	static glm::vec2 GetMouseSpeed();

	static void Update();

};

END_ENGINE
