#pragma once

#include "znmsp.h"

#include "Core/Ref.h"

#include <queue>
#include <mutex>

BEGIN_ENGINE

// Thin wrapper of a std::queue with a mutex
template<typename T>
class ThreadSafeQueue {
public:

	void Enqueue(const T& val)
	{
		std::scoped_lock l(mutex);
		queue.emplace(val);
	}

	T Dequeue()
	{
		std::scoped_lock l(mutex);
		T val = queue.front();
		queue.pop();
		return val;
	}

	bool Empty()
	{
		std::scoped_lock l(mutex);
		return queue.empty();
	}

private:
	std::mutex mutex;
	std::queue<T> queue;
};

END_ENGINE