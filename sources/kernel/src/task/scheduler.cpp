#include "task/scheduler.hpp"

OsStatus task::Scheduler::addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept {
    mQueues.insert({coreId, queue});
    return OsStatusSuccess;
}

OsStatus task::Scheduler::addTask(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept {
    for (auto& [_, queue] : mQueues) {
        if (queue->enqueue(state, userStack, kernelStack, entry) == OsStatusSuccess) {
            return OsStatusSuccess;
        }
    }

    return OsStatusOutOfMemory;
}

OsStatus task::Scheduler::create(Scheduler *scheduler) noexcept {
    scheduler->mQueues.clear();
    return OsStatusSuccess;
}
