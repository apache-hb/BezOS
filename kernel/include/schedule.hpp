#pragma once

#include "crt.hpp"
#include "isr.hpp"
#include "process/process.hpp"

#define MOODYCAMEL_MALLOC ::malloc
#define MOODYCAMEL_FREE ::free

#include <concurrentqueue.h>

namespace km {
    struct Task {
        uint32_t processId;
        uint32_t threadId;
        x64::RegisterState regs;
        x64::MachineState machine;
    };

    class Scheduler {
        moodycamel::ConcurrentQueue<ThreadId> mQueue;

    public:
        Scheduler();

        void addWorkItem(km::ThreadId thread);
        km::ThreadId getWorkItem();
    };

    void InitScheduler(IsrAllocator& isrs);
}
