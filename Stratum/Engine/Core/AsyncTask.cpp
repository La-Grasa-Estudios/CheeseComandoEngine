#include "AsyncTask.h"

using namespace ENGINE_NAMESPACE;

void AsyncTask::Callback()
{
	if (onCompleteCallback) {
		onCompleteCallback();
	}
}
