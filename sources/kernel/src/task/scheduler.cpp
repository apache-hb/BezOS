#include "task/scheduler.hpp"

#include "panic.hpp"

void task::SchedulerEntry::terminate() noexcept {
    TaskStatus expected = TaskStatus::eIdle;
    while (!status.compare_exchange_strong(expected, TaskStatus::eTerminated)) {
        switch (expected) {
        case task::TaskStatus::eRunning:
        case task::TaskStatus::eIdle:
        case task::TaskStatus::eSuspended:
            // If the task is suspended, retry the termination.
            continue;

        case task::TaskStatus::eTerminated:
            // If the task is already terminated, we can just return.
        case task::TaskStatus::eClosed:
            // Closed tasks can be terminated again if there are race conditions in userland.
            return;
        }
    }
}

bool task::SchedulerEntry::isClosed() const noexcept {
    return status.load() == TaskStatus::eClosed;
}

bool task::SchedulerQueue::moveTaskToIdle(SchedulerEntry *task) noexcept {
    TaskStatus expected = TaskStatus::eRunning;
    if (!task->status.compare_exchange_strong(expected, TaskStatus::eIdle)) {
        switch (expected) {
        case task::TaskStatus::eRunning:
            KM_PANIC("Task is already running, compare exchange should not fail");
            break;
        case task::TaskStatus::eIdle:
            KM_PANIC("Task is already idle, should not have been in the queue");
            break;
        case task::TaskStatus::eSuspended:
            // If the task is suspended, it should be left in this state.
            return false;
        case task::TaskStatus::eTerminated:
            // If the task is terminated, it should be moved to the closed state
            // and removed from the queue.
            task->status.store(TaskStatus::eClosed);
            return false;
        case task::TaskStatus::eClosed:
            KM_PANIC("Closed task should not be in the queue");
            break;
        }
    }

    return true;
}

bool task::SchedulerQueue::moveTaskToRunning(SchedulerEntry *task) noexcept {
    TaskStatus expected = TaskStatus::eIdle;
    if (!task->status.compare_exchange_strong(expected, TaskStatus::eRunning)) {
        switch (expected) {
        case task::TaskStatus::eRunning:
            KM_PANIC("Task is already running, should not be scheduled again");
            break;
        case task::TaskStatus::eIdle:
            KM_PANIC("Task is already idle, should not have been in the queue");
            break;
        case task::TaskStatus::eSuspended:
            // If the task is suspended, it should be left in this state.
            return false;
        case task::TaskStatus::eTerminated:
            // If the task is terminated, it should be moved to the closed state
            // and removed from the queue.
            task->status.store(TaskStatus::eClosed);
            return false;
        case task::TaskStatus::eClosed:
            KM_PANIC("Closed task should not be in the queue");
            break;
        }
    }

    return true;
}

bool task::SchedulerQueue::keepTaskRunning(SchedulerEntry *task) noexcept {
    switch (task->status.load()) {
    case TaskStatus::eRunning:
        // The task is already running, no need to change its state.
        return true;
    case TaskStatus::eIdle:
        KM_PANIC("Task is idle, should not be the current task");
        return false;
    case TaskStatus::eSuspended:
        // If the task is suspended, it should not be moved to running.
        return false;
    case TaskStatus::eTerminated:
        // If the task is terminated, it should be moved to the closed state
        // and removed from the queue.
        task->status.store(TaskStatus::eClosed);
        return false;
    case TaskStatus::eClosed:
        // Closed tasks should not be in the queue, this is a bug.
        KM_PANIC("Closed task should not be in the queue");
        return false;
    }
}

void task::SchedulerQueue::setCurrentTask(SchedulerEntry *task) noexcept {
    if (mCurrentTask != nullptr) {
        bool addToQueue = moveTaskToIdle(mCurrentTask);

        if (addToQueue) {
            if (!mQueue.tryPush(mCurrentTask)) {
                KM_PANIC("Failed to push old state back to the queue");
            }
        }
    }

    mCurrentTask = task;
}

bool task::SchedulerQueue::takeNextTask(SchedulerEntry **next) noexcept {
    SchedulerEntry *newTask;

    while (mQueue.tryPop(newTask)) {
        if (moveTaskToRunning(newTask)) {
            *next = newTask;
            return true;
        }
    }

    return false;
}

task::ScheduleResult task::SchedulerQueue::reschedule(TaskState *state [[gnu::nonnull]]) noexcept {
    SchedulerEntry *newTask;
    if (!takeNextTask(&newTask)) {
        //
        // This handles the case where there are no tasks in the queue.
        // If nothing at all is scheduled then we should idle the CPU.
        // If there is a current task we need to check if it can continue running.
        //
        if (mCurrentTask == nullptr) {
            return ScheduleResult::eIdle;
        }

        if (!keepTaskRunning(mCurrentTask)) {
            mCurrentTask->state = *state;
            mCurrentTask = nullptr;
            return ScheduleResult::eIdle;
        }

        return ScheduleResult::eResume;
    }

    if (mCurrentTask != nullptr) {
        mCurrentTask->state = *state;
    }

    setCurrentTask(newTask);

    *state = newTask->state;
    return ScheduleResult::eResume;
}

OsStatus task::SchedulerQueue::enqueue(const TaskState &state, km::StackMappingAllocation userStack, km::StackMappingAllocation kernelStack, SchedulerEntry *entry) noexcept {
    KM_ASSERT(entry->status.load() == TaskStatus::eIdle);

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
