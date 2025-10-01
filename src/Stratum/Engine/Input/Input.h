#pragma once

#define MAX_KEYS 1000
#define MAX_BUTTONS 64

#include "znmsp.h"
#include "KeyCode.h"
#include "InputLayer.h"

#include <glm/glm.hpp>
#include <unordered_set>
#include <unordered_map>
#include <vector>

struct SDL_Window;

BEGIN_ENGINE

class BaseInputLayer : public InputLayer_Interface
{
public:
	bool SetKey(int key, bool press) final;
	bool SetMouse(int click, bool press) final;
	bool SetGamepad(int button, bool press) final;
	bool SetGamepadAxis(int axis, int16_t value) final;
};

enum class MouseInputMode {
	Normal,
	Hidden,
	Disabled,
};

enum class MouseButton
{
	LEFT,
	RIGHT,
	MIDDLE,
	MOUSE4,
	MOUSE5,
	MOUSE6,
	MOUSE7
};

class Input {
	friend BaseInputLayer;

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
	static void SetGamepadRumble(float left_intensity, float right_intensity, uint32_t duration);

	// Legacy Stuff

	static void SetInputMode(MouseInputMode mode);

	static bool GetKeyDown(KeyCode keyCode);
	static bool GetKey(KeyCode keyCode);

	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);

	static bool GetGamepadButton(GamepadButton button);
	static bool GetGamepadButtonDown(GamepadButton button);

	static float GetGamepadAxis(GamepadAxis axis);

	static int32_t GetGamepadType();

	static void BindAlias(const char* alias, KeyCode keyCode);
	static void BindAlias(const char* alias, MouseButton button);
	static void BindAlias(const char* alias, GamepadButton gamepadButton);
	static void BindAxisToAlias(const char* alias, GamepadAxis axis, float activation = 0.5f);

	static void UnBindAlias(const char* alias, KeyCode keyCode);
	static void UnBindAlias(const char* alias, MouseButton button);
	static void UnBindAlias(const char* alias, GamepadButton gamepadButton);
	static void UnBindAxisFromAlias(const char* alias, GamepadAxis axis);

	static bool GetInput(const char* alias);
	static bool GetInputDown(const char* alias);
	static bool GetInputUp(const char* alias);

	static float GetAxis(const char* alias);

	static bool AnyKeyDown();
	static bool AnyGamepadDown();
	static bool HasGamepadConnected();

	static glm::vec2 GetMousePosition();
	static glm::vec2 GetMouseSpeed();

	static void PushInputLayer(InputLayer_Interface* inputLayer);
	static void PopInputLayer();

	static void Update();

private:

	struct InputAlias
	{
		std::unordered_set<KeyCode> keyCodes;
		std::unordered_set<MouseButton> mouseButtons;
		std::unordered_set<int32_t> gamepadButtons;
		std::unordered_set<int32_t> gpadAxis;
		bool active = false;
	};

	static InputAlias* GetInputAlias(const char* name);

	inline static std::unordered_map<const char*, InputAlias> sAliases;
	inline static std::vector<InputLayer_Interface*> sLayers;

};

END_ENGINE
