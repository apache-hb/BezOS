#include "task/mutex.hpp"

OsStatus task::Mutex::wait(SchedulerEntry *entry, km::os_instant timeout) noexcept {
    WaitEntry waitEntry {
        .entry = entry,
        .timeout = timeout
    };

    return mWaiters.add(waitEntry);
}

OsStatus task::Mutex::alert() noexcept {
    return OsStatusSuccess;
}
