#include "task/scheduler.hpp"

#include "panic.hpp"

void task::SchedulerQueue::setCurrentTask(SchedulerEntry *task) noexcept {
    task->status.store(TaskStatus::eRunning);
    if (mCurrentTask != nullptr) {
        mCurrentTask->status.store(TaskStatus::eIdle);
    }

    mCurrentTask = task;
}

task::ScheduleResult task::SchedulerQueue::reschedule(TaskState *state [[gnu::nonnull]]) noexcept {
    SchedulerEntry *newTask;
    if (!mQueue.tryPop(newTask)) {
        return task::ScheduleResult::eResume;
    }

    if (newTask == nullptr) {
        return task::ScheduleResult::eResume;
    }

    if (mCurrentTask != nullptr) {
        mCurrentTask->status.store(TaskStatus::eIdle);
        mCurrentTask->state = *state;

        if (!mQueue.tryPush(mCurrentTask)) {
            KM_CHECK(false, "Failed to push old state back to the queue");
        }
    }

    setCurrentTask(newTask);

    *state = newTask->state;
    return task::ScheduleResult::eResume;
}

OsStatus task::SchedulerQueue::enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept {
    entry->status.store(TaskStatus::eIdle);
    entry->state = state;
    entry->userStack = userStack;
    entry->kernelStack = kernelStack;

    if (!mQueue.tryPush(entry)) {
        return OsStatusOutOfMemory;
    }

    return OsStatusSuccess;
}

OsStatus task::SchedulerQueue::create(uint32_t capacity, SchedulerQueue *queue) noexcept {
    queue->mCurrentTask = nullptr;
    return EntryQueue::create(capacity, &queue->mQueue);
}
