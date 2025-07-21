#pragma once

#include <stddef.h>

#include "clock.hpp"

#include "std/vector.hpp"

namespace task {
    class SchedulerEntry;

    class Mutex {
        stdx::Vector2<SchedulerEntry*> mWaiters;

    public:
        constexpr Mutex() noexcept = default;

        OsStatus wait(SchedulerEntry *entry, km::os_instant timeout) noexcept;

        OsStatus alert() noexcept;
    };
}
