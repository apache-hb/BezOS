#pragma once

#include "crt.hpp"
#include "process/process.hpp"

#include "std/queue.hpp"

namespace km {
    class Scheduler {
        moodycamel::ConcurrentQueue<sm::RcuWeakPtr<Thread>> mQueue;

    public:
        Scheduler();

        void addWorkItem(sm::RcuSharedPtr<Thread> thread);
        sm::RcuSharedPtr<km::Thread> getWorkItem();
    };

    sm::RcuSharedPtr<Thread> GetCurrentThread();
    sm::RcuSharedPtr<Process> GetCurrentProcess();

    [[noreturn]]
    void SwitchThread(sm::RcuSharedPtr<Thread> next);

    [[noreturn]]
    void ScheduleWork(LocalIsrTable *table, IApic *apic, sm::RcuSharedPtr<km::Thread> initial = nullptr);

    void YieldCurrentThread();
}
