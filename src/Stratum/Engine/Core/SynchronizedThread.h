#pragma once

#include "znmsp.h"
#include <thread>
#include <semaphore>

static void thread_cycle(std::binary_semaphore* s, std::binary_semaphore* s1, bool* r, void (*fn)(uintptr_t*), uintptr_t* p) {
	while (*r) {
		s->acquire();
		fn(p);
		s1->release();
	}
}

BEGIN_ENGINE

class SynchronizedThread {

	std::binary_semaphore m_semaphore{ 0 };
	std::binary_semaphore m_semaphore_update{ 0 };
	std::thread* m_thread;
	bool m_IsRunning = true;
	bool f = true;

public:
	SynchronizedThread(void(*fn)(uintptr_t*), uintptr_t* pData) {
		m_semaphore_update.release();
		m_thread = new std::thread(thread_cycle, &m_semaphore, &m_semaphore_update, &m_IsRunning, fn, pData);
	}

	void Pump() {
		m_semaphore.release();
	}

	void Wait() {
		m_semaphore_update.lock();
	}

	void Stop() {
		m_IsRunning = false;
		Pump();
	}

	~SynchronizedThread() {
		m_IsRunning = false;
		Pump();
		m_thread->join();
	}

};

END_ENGINE
