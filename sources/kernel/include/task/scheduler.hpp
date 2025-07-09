#pragma once

#include "processor.hpp"
#include "task/scheduler_queue.hpp"

namespace task {
    class Scheduler {
        struct QueueInfo {
            SchedulerQueue *queue;
        };

        sm::AbslBTreeMap<km::CpuCoreId, QueueInfo> mQueues;

    public:
        OsStatus addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept;
        OsStatus enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept;

        ScheduleResult reschedule(km::CpuCoreId coreId, TaskState *state) noexcept;

        static OsStatus create(Scheduler *scheduler) noexcept;
    };
}
