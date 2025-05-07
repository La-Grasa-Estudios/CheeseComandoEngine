#pragma once

#include "znmsp.h"

#include <vector>
#include <functional>
#include <mutex>

BEGIN_ENGINE

struct JobDispatchArgs
{
    uint32_t jobIndex;
    uint32_t groupIndex;
    bool IsFirstJobInGroup = false;
    bool IsLastJobInGroup = false;
    void* sharedMemory;
};

class JobManager {

public:

    static void Init(bool isSerial);

    static void EnqueueMainThread(const std::function<void()>& func);

    static void Execute(const std::function<void()>& job);
    static void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job, size_t sharedMemorySize = 1);

    static void ExecuteMainJobs();

    static bool IsBusy();
    static void Wait();

private:

    inline static JobManager* s_Instance = NULL;

    std::vector<std::function<void()>> m_MainQueue;

    std::mutex m_MainMutex;

    uint32_t m_NbThreads = 0;

};

END_ENGINE
