#pragma once

#include "processor.hpp"
#include "task/scheduler_queue.hpp"

namespace task {
    class Scheduler {
        sm::BTreeMap<km::CpuCoreId, SchedulerQueue> mQueues;

    public:
    };
}
