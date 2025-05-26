#pragma once

#include "std/ringbuffer.hpp"
namespace task {
    /// @brief All state required to suspend or resume a thread.
    struct TaskState {

    };

    struct SchedulerEntry {
        TaskState state;
    };

    class SchedulerQueue {
        sm::AtomicRingQueue<SchedulerEntry> mQueue;

    public:
        constexpr SchedulerQueue() noexcept = default;

        static OsStatus create(uint32_t capacity, SchedulerQueue *queue [[gnu::nonnull]]) noexcept;
    };

    class Scheduler {

    };
}
