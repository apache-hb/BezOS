#include "task/mutex.hpp"
#include "task/scheduler_queue.hpp"

OsStatus task::Mutex::wait(SchedulerEntry *entry, km::os_instant timeout) noexcept {
    if (!entry->sleep(timeout)) {
        return OsStatusThreadTerminated;
    }

    if (OsStatus status = mWaiters.add(entry)) {
        KM_CHECK(status == OsStatusSuccess, "Failed to add wait entry to mutex waiters");
    }

    return OsStatusSuccess;
}

OsStatus task::Mutex::alert() noexcept {
    for (SchedulerEntry *entry : mWaiters) {
        entry->wake();
    }
    mWaiters.clear();
    return OsStatusSuccess;
}
