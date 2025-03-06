#pragma once

#include "crt.hpp"
#include "process/process.hpp"

namespace km {
    struct SchedulerQueueTraits : moodycamel::ConcurrentQueueDefaultTraits {
        static void *malloc(size_t size);
        static void free(void *ptr);
    };

    class Scheduler {
        moodycamel::ConcurrentQueue<sm::RcuWeakPtr<Thread>, SchedulerQueueTraits> mQueue;

    public:
        Scheduler();

        void addWorkItem(sm::RcuSharedPtr<Thread> thread);
        sm::RcuSharedPtr<km::Thread> getWorkItem() noexcept;
    };

    void InitSchedulerMemory(void *memory, size_t size);

    sm::RcuSharedPtr<Thread> GetCurrentThread();
    sm::RcuSharedPtr<Process> GetCurrentProcess();

    [[noreturn]]
    void SwitchThread(sm::RcuSharedPtr<Thread> next);

    void InstallSchedulerIsr(LocalIsrTable *table);

    [[noreturn]]
    void ScheduleWork(LocalIsrTable *table, IApic *apic, sm::RcuSharedPtr<km::Thread> initial = nullptr);

    void YieldCurrentThread();
}
