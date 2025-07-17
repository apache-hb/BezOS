#pragma once

#include <stddef.h>

#include "clock.hpp"

#include "std/vector.hpp"

namespace task {
    class SchedulerEntry;

    class Mutex {
        struct WaitEntry {
            SchedulerEntry *entry;
            km::os_instant timeout;

            bool operator<(const WaitEntry& other) const noexcept {
                return timeout < other.timeout;
            }
        };

        stdx::Vector2<WaitEntry> mWaiters;

    public:
        constexpr Mutex() noexcept = default;

        OsStatus wait(SchedulerEntry *entry, km::os_instant timeout) noexcept;

        OsStatus alert() noexcept;
    };
}
