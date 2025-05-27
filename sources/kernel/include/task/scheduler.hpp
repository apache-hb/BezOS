#pragma once

#include "memory/stack_mapping.hpp"
#include "std/ringbuffer.hpp"
#include "system/create.hpp"

namespace task {
    /// @brief All state required to suspend or resume a thread.
    struct TaskState {
        sys::RegisterSet registers;
        km::PhysicalAddress pageMap;
        km::StackMappingAllocation userStack;
        km::StackMappingAllocation kernelStack;
    };

    struct SchedulerEntry {
        TaskState state;
    };

    /// @brief What should be done with the result of a scheduling operation.
    enum class ScheduleResult {
        /// @brief Run the task.
        eResume,

        /// @brief No task is available, the current core should sleep until the next scheduler interrupt.
        eIdle,
    };

    class SchedulerQueue {
        using EntryQueue = sm::AtomicRingQueue<SchedulerEntry>;
        EntryQueue mQueue;

    public:
        constexpr SchedulerQueue() noexcept = default;

        /// @brief Reschedule the current thread.
        ///
        /// @param[in,out] state The old state of the thread, the new state will be written to this if
        ///                      the thread is rescheduled.
        ///
        /// @return If the thread was rescheduled.
        /// @retval true The thread was rescheduled.
        /// @retval false The thread was not rescheduled, it is still the current thread.
        ScheduleResult reschedule(TaskState *state [[gnu::nonnull]]) noexcept;

        /// @brief Enqueue a new task to the scheduler.
        OsStatus enqueue(const TaskState &state) noexcept;

        static OsStatus create(uint32_t capacity, SchedulerQueue *queue [[gnu::nonnull]]) noexcept;
    };

    class Scheduler {

    };
}
