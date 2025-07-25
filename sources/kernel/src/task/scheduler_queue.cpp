#include "task/scheduler_queue.hpp"

#include "logger/categories.hpp"
#include "panic.hpp"

template<typename T>
static void atomicMax(std::atomic<T> &target, T value) noexcept {
    T current = target.load();
    while (current < value && !target.compare_exchange_weak(current, value)) {
    }
}

task::WakeResult task::SchedulerEntry::wakeIfTimeout(km::os_instant now) noexcept {
    if (mSleepUntil.load() > now) {
        // If the task is still sleeping, do not change its state.
        return WakeResult::eSleep;
    }

    TaskStatus expected = TaskStatus::eSuspended;
    while (!mStatus.compare_exchange_strong(expected, TaskStatus::eIdle)) {
        switch (expected) {
        case task::TaskStatus::eRunning:
            KM_PANIC("Task is running, should not be in the sleep queue");
            continue;
        case task::TaskStatus::eSuspended:
            KM_PANIC("compare_exchange_strong should not fail");
            continue;
        case task::TaskStatus::eIdle:
            continue;
        case task::TaskStatus::eTerminated:
        case task::TaskStatus::eClosed:
            return WakeResult::eDiscard;
        }
    }

    return WakeResult::eWake;
}

void task::SchedulerEntry::wake() noexcept {
    mSleepUntil.store(km::os_instant::min());
}

bool task::SchedulerEntry::sleep(km::os_instant timeout) noexcept {
    TaskStatus expected = TaskStatus::eIdle;
    while (!mStatus.compare_exchange_strong(expected, TaskStatus::eSuspended)) {
        switch (expected) {
        case task::TaskStatus::eClosed:
        case task::TaskStatus::eTerminated:
            // If the task is closed or terminated then theres no point in sleeping it.
            return false;
        case task::TaskStatus::eRunning:
        case task::TaskStatus::eIdle:
        case task::TaskStatus::eSuspended:
            atomicMax(mSleepUntil, timeout);
            continue;
        }
    }

    return true;
}

void task::SchedulerEntry::terminate() noexcept {
    TaskStatus expected = TaskStatus::eIdle;
    while (!mStatus.compare_exchange_strong(expected, TaskStatus::eTerminated)) {
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
    return mStatus.load() == TaskStatus::eClosed;
}

bool task::SchedulerQueue::moveTaskToIdle(SchedulerEntry *task) noexcept {
    TaskStatus expected = TaskStatus::eRunning;
    if (!task->mStatus.compare_exchange_strong(expected, TaskStatus::eIdle)) {
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
            task->mStatus.store(TaskStatus::eClosed);
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
    if (!task->mStatus.compare_exchange_strong(expected, TaskStatus::eRunning)) {
        switch (expected) {
        case task::TaskStatus::eRunning:
            TaskLog.errorf("Task ", (void*)task, " is already running, it should not be scheduled again.");
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
            task->mStatus.store(TaskStatus::eClosed);
            return false;
        case task::TaskStatus::eClosed:
            KM_PANIC("Closed task should not be in the queue");
            break;
        }
    }

    return true;
}

bool task::SchedulerQueue::keepTaskRunning(SchedulerEntry *task) noexcept {
    switch (task->mStatus.load()) {
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
        task->mStatus.store(TaskStatus::eClosed);
        return false;
    case TaskStatus::eClosed:
        // Closed tasks should not be in the queue, this is a bug.
        KM_PANIC("Closed task should not be in the queue");
        return false;
    }
}

size_t task::SchedulerQueue::wakeSleepingTasks(km::os_instant now) noexcept {
    size_t woken = 0;

    for (auto it = mSleepingTasks.begin(); it != mSleepingTasks.end();) {
        SchedulerEntry *task = *it;
        task::WakeResult result = task->wakeIfTimeout(now);
        switch (result) {
        case task::WakeResult::eSleep:
            // The task is still sleeping, do not remove it from the list.
            ++it;
            continue;
        case task::WakeResult::eWake:
            woken += 1;
            // The task was woken up, remove it from the list.
            it = mSleepingTasks.erase(it);

            // This must never fail, if it does then this queue has been overfilled.
            KM_ASSERT(mQueue.tryPush(task));
            continue;
        case task::WakeResult::eDiscard:
            woken += 1;
            // The task was discarded, remove it from the list.
            it = mSleepingTasks.erase(it);
            continue;
        }
    }

    return woken;
}

void task::SchedulerQueue::setCurrentTask(SchedulerEntry *task) noexcept {
    KM_ASSERT(task->mStatus.load() == TaskStatus::eRunning);
    KM_ASSERT(task != mCurrentTask);

    if (mCurrentTask != nullptr) {
        bool addToQueue = moveTaskToIdle(mCurrentTask);

        if (addToQueue) {
            if (!mQueue.tryPush(mCurrentTask)) {
                SchedulerEntry *rescueTask = mRescueTask.exchange(mCurrentTask);
                KM_ASSERT(rescueTask == nullptr);
            }
        }
    }

    mCurrentTask = task;
}

bool task::SchedulerQueue::takeNextTask(SchedulerEntry **next) noexcept {
    SchedulerEntry *newTask;

    //
    // Take a task from the queue, drop any tasks that are not able to be scheduled.
    //
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
            mCurrentTask->mState = *state;
            mCurrentTask = nullptr;
            return ScheduleResult::eIdle;
        }

        return ScheduleResult::eResume;
    }

    if (mCurrentTask != nullptr) {
        mCurrentTask->mState = *state;
    }

    setCurrentTask(newTask);

    *state = newTask->mState;
    return ScheduleResult::eResume;
}

OsStatus task::SchedulerQueue::enqueue(const TaskState &state, SchedulerEntry *entry) noexcept {
    KM_ASSERT(entry->mStatus.load() == TaskStatus::eIdle);

    if (SchedulerEntry *rescueTask = mRescueTask.load()) {
        if (mQueue.tryPush(rescueTask)) {
            mRescueTask.store(nullptr);
        } else {
            return OsStatusOutOfMemory;
        }
    }

    entry->mState = state;

    if (!mQueue.tryPush(entry)) {
        return OsStatusOutOfMemory;
    }

    return OsStatusSuccess;
}

OsStatus task::SchedulerQueue::create(uint32_t capacity, SchedulerQueue *queue) noexcept {
    queue->mCurrentTask = nullptr;
    queue->mRescueTask = nullptr;
    return EntryQueue::create(capacity, &queue->mQueue);
}
