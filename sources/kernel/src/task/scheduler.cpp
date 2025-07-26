#include "task/scheduler.hpp"
#include "task/mutex.hpp"

OsStatus task::Scheduler::addQueue(km::CpuCoreId coreId, SchedulerQueue *queue) noexcept {
    if (OsStatus status = mQueues.insert(coreId, QueueInfo{queue})) {
        return status;
    }
    mAvailableTaskCount.add(queue->getCapacity(), std::memory_order_relaxed);
    return OsStatusSuccess;
}

OsStatus task::Scheduler::enqueue(const TaskState &state, SchedulerEntry *entry) noexcept {
    if (!mAvailableTaskCount.consume()) {
        return OsStatusOutOfMemory;
    }

    uint64_t taskId = mNextTaskId.fetch_add(1, std::memory_order_relaxed);
    entry->mId = taskId;

    for (auto taskQueue : mQueues) {
        SchedulerQueue *queue = taskQueue.second.queue;
        if (queue->enqueue(state, entry) == OsStatusSuccess) {
            return OsStatusSuccess;
        }
    }

    mAvailableTaskCount.add(1);
    return OsStatusOutOfMemory;
}

task::SchedulerQueue *task::Scheduler::getQueue(km::CpuCoreId coreId) noexcept {
    auto it = mQueues.find(coreId);
    KM_ASSERT(it != mQueues.end());
    return (*it).second.queue;
}

task::ScheduleResult task::Scheduler::reschedule(km::CpuCoreId coreId, TaskState *state) noexcept {
    auto it = mQueues.find(coreId);
    KM_ASSERT(it != mQueues.end());

    SchedulerQueue *queue = (*it).second.queue;

    return queue->reschedule(state);
}

OsStatus task::Scheduler::sleep(SchedulerEntry *entry, km::os_instant timeout) noexcept {
    return entry->sleep(timeout) ? OsStatusSuccess : OsStatusThreadTerminated;
}

OsStatus task::Scheduler::wait(SchedulerEntry *entry, Mutex *waitable, km::os_instant timeout) noexcept {
    return waitable->wait(entry, timeout);
}

OsStatus task::Scheduler::create(Scheduler *scheduler) noexcept {
    scheduler->mQueues.clear();
    return OsStatusSuccess;
}
