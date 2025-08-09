#pragma once

#include "znmsp.h"

#include <glm/glm.hpp>

BEGIN_ENGINE

/// <summary>
/// Input stack implementation.
/// Override these methods to add a new input layer.
/// Return true when input should not be longer propogated down the stack
/// </summary>
class InputLayer_Interface
{
public:
	virtual bool SetKey(int key, bool press) = 0;
	virtual bool SetMouse(int click, bool press) = 0;
	virtual bool SetGamepad(int button, bool press) = 0;
	virtual bool SetGamepadAxis(int axis, int16_t value) = 0;
};

END_ENGINE