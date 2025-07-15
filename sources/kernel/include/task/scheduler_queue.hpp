#pragma once

#include "arch/xsave.hpp"
#include "memory/stack_mapping.hpp"
#include "std/ringbuffer.hpp"
#include "system/create.hpp"

namespace task {
    class Waitable;
    class SchedulerQueue;

    /// @brief Task status state transitions.
    enum class TaskStatus {
        /// @brief The task is not currently running but can be resumed.
        eIdle,

        /// @brief The task is currently running on a core.
        eRunning,

        /// @brief The task is currently waiting for a resource or event.
        eSuspended,

        /// @brief Terminal state, once a task reaches this state it can not be resumed.
        /// @note A task with this state can still be executing, but it will not be scheduled again.
        eTerminated,

        /// @brief The task is closed, it has been removed from the scheduler queue and will not be scheduled again.
        eClosed,
    };

    /// @brief All state required to suspend or resume a thread.
    struct TaskState {
        sys::RegisterSet registers;
        x64::XSave *xsave;
        km::PhysicalAddress pageMap;
        uintptr_t tlsBase;
    };

    class SchedulerEntry {
        friend class SchedulerQueue;

        std::atomic<TaskStatus> status{TaskStatus::eIdle};
        TaskState state;
        km::StackMappingAllocation userStack;
        km::StackMappingAllocation kernelStack;

    public:
        constexpr SchedulerEntry() noexcept = default;

        void terminate() noexcept;
        bool isClosed() const noexcept;

        TaskState& getState() noexcept {
            return state;
        }
    };

    /// @brief What should be done with the result of a scheduling operation.
    enum class ScheduleResult {
        /// @brief Run the task.
        eResume,

        /// @brief No task is available, the current core should sleep until the next scheduler interrupt.
        eIdle,
    };

    class SchedulerQueue {
        using EntryQueue = sm::AtomicRingQueue<SchedulerEntry*>;
        EntryQueue mQueue;
        SchedulerEntry *mCurrentTask;

        /// @brief A rescue task that stores a task when the scheduler queue is totally full.
        /// Prevents leaking the current task when the queue is full.
        std::atomic<SchedulerEntry*> mRescueTask;

        void setCurrentTask(SchedulerEntry *task) noexcept;

        bool takeNextTask(SchedulerEntry **next) noexcept;

        /// @brief Try to transition a task to the idle state.
        ///
        /// @return If the task was successfully moved to the idle state.
        /// @retval true The task was moved to the idle state.
        /// @retval false The task was not moved, it should be dropped from the queue.
        static bool moveTaskToIdle(SchedulerEntry *task) noexcept;

        /// @brief Try to transition a task to the running state.
        ///
        /// @return If the task was successfully moved to the running state.
        /// @retval true The task was moved to the running state.
        /// @retval false The task was not moved, it should be dropped from the queue.
        static bool moveTaskToRunning(SchedulerEntry *task) noexcept;

        /// @brief Decide if a task should continue running or if it needs to be dequeued.
        ///
        /// @return If the task should continue running.
        /// @retval true The task should continue running.
        /// @retval false The task should be dequeued.
        static bool keepTaskRunning(SchedulerEntry *task) noexcept;

    public:
        constexpr SchedulerQueue() noexcept
            : mCurrentTask(nullptr)
            , mRescueTask(nullptr)
        { }

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
        ///
        /// @warning @p entry must have a stable address, it will be stored in the queue.
        OsStatus enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept;

        size_t getTaskCount() const noexcept {
            return mQueue.count();
        }

        size_t getCapacity() const noexcept {
            return mQueue.capacity();
        }

        SchedulerEntry *getCurrentTask() const noexcept [[clang::reentrant]] {
            return mCurrentTask;
        }

        static OsStatus create(uint32_t capacity, SchedulerQueue *queue [[gnu::nonnull]]) noexcept;
    };
}
