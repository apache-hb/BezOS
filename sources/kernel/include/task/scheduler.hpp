#pragma once

#include "processor.hpp"
#include "task/scheduler_queue.hpp"

namespace task {
    class Scheduler {
        sm::AbslBTreeMap<km::CpuCoreId, SchedulerQueue*> mQueues;

    public:
        OsStatus addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept;
        OsStatus addTask(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept;

        static OsStatus create(Scheduler *scheduler) noexcept;
    };
}
