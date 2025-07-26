#pragma once

#include "processor.hpp"
#include "std/container/btree.hpp"
#include "task/scheduler_queue.hpp"

#include "clock.hpp"

namespace task {
    class Mutex;

    class AvailableTaskCount {
        std::atomic<int32_t> mAvailable;

    public:
        constexpr AvailableTaskCount(uint32_t initialCount = 0) noexcept
            : mAvailable(initialCount)
        { }

        constexpr AvailableTaskCount(AvailableTaskCount &&other) noexcept
            : mAvailable(other.mAvailable.load())
        { }

        bool consume() noexcept {
            int32_t current = mAvailable.fetch_sub(1);
            if (current > 0) {
                return true;
            }

            mAvailable.fetch_add(1); // Restore the count if we went below zero
            return false; // No tasks available
        }

        void add(uint32_t count, std::memory_order order = std::memory_order_seq_cst) noexcept {
            mAvailable.fetch_add(count, order);
        }

        uint32_t available(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return std::max(0, mAvailable.load(order));
        }
    };

    class Scheduler {
        struct QueueInfo {
            SchedulerQueue *queue;
        };

        sm::BTreeMap<km::CpuCoreId, QueueInfo> mQueues;
        AvailableTaskCount mAvailableTaskCount;
        std::atomic<uint64_t> mNextTaskId{1};

    public:
        Scheduler() = default;

        Scheduler(Scheduler &&other) noexcept
            : mQueues(std::move(other.mQueues))
            , mAvailableTaskCount(std::move(other.mAvailableTaskCount))
            , mNextTaskId(other.mNextTaskId.load())
        { }

        constexpr Scheduler &operator=(Scheduler&& other) noexcept = delete;

        OsStatus addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept;
        OsStatus enqueue(const TaskState &state, SchedulerEntry *entry) noexcept;
        SchedulerQueue *getQueue(km::CpuCoreId coreId) noexcept;

        ScheduleResult reschedule(km::CpuCoreId coreId, TaskState *state) noexcept;
        ScheduleResult reschedule(SchedulerQueue *queue, TaskState *state) noexcept;

        OsStatus sleep(SchedulerEntry *entry, km::os_instant timeout) noexcept;
        OsStatus wait(SchedulerEntry *entry, Mutex *waitable, km::os_instant timeout) noexcept;

        static OsStatus create(Scheduler *scheduler) noexcept;
    };
}
