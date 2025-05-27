#include "task/scheduler.hpp"

#include "panic.hpp"

task::ScheduleResult task::SchedulerQueue::reschedule(TaskState *state [[gnu::nonnull]]) noexcept {
    SchedulerEntry newState;
    if (!mQueue.tryPop(newState)) {
        return task::ScheduleResult::eResume;
    }

    SchedulerEntry oldState = { *state };
    if (!mQueue.tryPush(oldState)) {
        KM_CHECK(false, "Failed to push old state back to the queue");
    }

    *state = newState.state;
    return task::ScheduleResult::eResume;
}

OsStatus task::SchedulerQueue::enqueue(const TaskState &state) noexcept {
    SchedulerEntry entry{ .state = state };
    if (!mQueue.tryPush(entry)) {
        return OsStatusOutOfMemory;
    }
    return OsStatusSuccess;
}

OsStatus task::SchedulerQueue::create(uint32_t capacity, SchedulerQueue *queue) noexcept {
    return EntryQueue::create(capacity, &queue->mQueue);
}
