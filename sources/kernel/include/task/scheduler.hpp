#pragma once

#include "processor.hpp"
#include "std/container/btree.hpp"
#include "task/scheduler_queue.hpp"

#include "clock.hpp"

namespace task {
    class Mutex;

    class AvailableTaskCount {
        std::atomic<uint32_t> mAvailable{0};

    public:
        bool consume() noexcept {
            while (true) {
                uint32_t current = mAvailable.load();
                if (current == 0) {
                    return false; // No tasks available
                }

                if (mAvailable.compare_exchange_weak(current, current - 1)) {
                    return true; // Successfully consumed a task
                }
            }
        }

        void add(uint32_t count, std::memory_order order = std::memory_order_seq_cst) noexcept {
            mAvailable.fetch_add(count, order);
        }

        uint32_t available(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return mAvailable.load(order);
        }
    };

    class Scheduler {
        struct QueueInfo {
            SchedulerQueue *queue;
        };

        sm::BTreeMap<km::CpuCoreId, QueueInfo> mQueues;
        AvailableTaskCount mAvailableTaskCount;

    public:
        OsStatus addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept;
        OsStatus enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept;
        SchedulerQueue *getQueue(km::CpuCoreId coreId) noexcept;

        ScheduleResult reschedule(km::CpuCoreId coreId, TaskState *state) noexcept;

        OsStatus sleep(SchedulerEntry *entry, km::os_instant timeout) noexcept;
        OsStatus wait(SchedulerEntry *entry, Mutex *waitable, km::os_instant timeout) noexcept;

        static OsStatus create(Scheduler *scheduler) noexcept;
    };
}
