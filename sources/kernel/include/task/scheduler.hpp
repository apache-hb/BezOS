#pragma once

#include "task/scheduler_queue.hpp"

namespace task {
    class Scheduler {
        sm::BTreeMap<int, SchedulerQueue> mQueues;

    public:
    };
}
