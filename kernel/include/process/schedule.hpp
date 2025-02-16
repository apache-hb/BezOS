#pragma once

#include "crt.hpp"
#include "process/process.hpp"

#define MOODYCAMEL_MALLOC ::malloc
#define MOODYCAMEL_FREE ::free

#include <concurrentqueue.h>

namespace km {
    class Scheduler {
        moodycamel::ConcurrentQueue<Thread*> mQueue;

    public:
        Scheduler();

        void addWorkItem(km::Thread *thread);
        km::Thread *getWorkItem();
    };

    km::Thread *GetCurrentThread();
    km::Process *GetCurrentProcess();

    [[noreturn]]
    void ScheduleWork(IsrTable *table, IApic *apic);
}
