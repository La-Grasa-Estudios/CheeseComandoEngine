#pragma once

#include "znmsp.h"
#include <functional>

BEGIN_ENGINE

enum class TaskState {
	PROCESSING,
	COMPLETE,
	TASK_ERROR,
};

struct AsyncTask {
	TaskState state = TaskState::PROCESSING;
	std::function<void()> onCompleteCallback;
	void Callback();
};

END_ENGINE