#include "task/scheduler.hpp"

OsStatus task::Scheduler::addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept {
    mQueues.insert({coreId, QueueInfo{queue}});
    return OsStatusSuccess;
}

OsStatus task::Scheduler::enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept {
    for (auto& [_, queueInfo] : mQueues) {
        SchedulerQueue *queue = queueInfo.queue;
        if (queue->enqueue(state, userStack, kernelStack, entry) == OsStatusSuccess) {
            return OsStatusSuccess;
        }
    }

    return OsStatusOutOfMemory;
}

task::SchedulerQueue *task::Scheduler::getQueue(km::CpuCoreId coreId) noexcept {
    auto it = mQueues.find(coreId);
    KM_ASSERT(it != mQueues.end());
    return it->second.queue;
}

task::ScheduleResult task::Scheduler::reschedule(km::CpuCoreId coreId, TaskState *state) noexcept {
    auto it = mQueues.find(coreId);
    KM_ASSERT(it != mQueues.end());

    auto& [_, queueInfo] = *it;
    SchedulerQueue *queue = queueInfo.queue;

    return queue->reschedule(state);
}

OsStatus task::Scheduler::create(Scheduler *scheduler) noexcept {
    scheduler->mQueues.clear();
    return OsStatusSuccess;
}
