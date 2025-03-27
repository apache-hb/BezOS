#pragma once

#include "isr/isr.hpp"

#include "std/queue.hpp" // IWYU pragma: keep
#include "crt.hpp" // IWYU pragma: keep

#include "process/process.hpp"

#include "std/spinlock.hpp"

namespace km {
    struct SchedulerQueueTraits : moodycamel::ConcurrentQueueDefaultTraits {
        static void *malloc(size_t size);
        static void free(void *ptr);
    };

    namespace detail {
        /// @brief Per CPU core scheduler state
        class CpuScheduler {
            stdx::SpinLock mLock;

        public:
        };
    }

    class Scheduler {
        moodycamel::ConcurrentQueue<Thread*, SchedulerQueueTraits> mQueue;

    public:
        Scheduler();

        void addWorkItem(km::Thread *thread);
        km::Thread *getWorkItem() noexcept;
    };

    void InitSchedulerMemory(void *memory, size_t size);

    km::Thread *GetCurrentThread();
    Process *GetCurrentProcess();

    void InstallSchedulerIsr(LocalIsrTable *table);

    [[noreturn]]
    void ScheduleWork(LocalIsrTable *table, IApic *apic, km::Thread *initial = nullptr);

    void YieldCurrentThread();
}
