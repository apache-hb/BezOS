#pragma once

#include "std/queue.hpp"

#include "crt.hpp"
#include "process/process.hpp"

namespace km {
    struct SchedulerQueueTraits : moodycamel::ConcurrentQueueDefaultTraits {
        static void *malloc(size_t size);
        static void free(void *ptr);
    };

    class Scheduler {
        moodycamel::ConcurrentQueue<Thread*, SchedulerQueueTraits> mQueue;

    public:
        Scheduler();

        void addWorkItem(km::Thread *thread);
        km::Thread *getWorkItem() noexcept;
    };

    void InitSchedulerMemory(void *memory, size_t size);

    km::Thread *GetCurrentThread();
    Process * GetCurrentProcess();

    [[noreturn]]
    void SwitchThread(Thread *next);

    void InstallSchedulerIsr(LocalIsrTable *table);

    [[noreturn]]
    void ScheduleWork(LocalIsrTable *table, IApic *apic, km::Thread *initial = nullptr);

    void YieldCurrentThread();
}
