#pragma once

#include <bezos/handle.h>

#include "util/absl.hpp"

#include <compare>
#include <queue>

namespace sys2 {
    class IObject;
    class Process;
    class Thread;

    struct WaitEntry {
        OsInstant timeout;
        Thread *thread;

        constexpr auto operator<=>(const WaitEntry& other) const noexcept {
            return timeout <=> other.timeout;
        }
    };

    struct TimeoutEntry {
        OsInstant timeout;
        IObject *object;

        constexpr auto operator<=>(const TimeoutEntry& other) const noexcept {
            return timeout <=> other.timeout;
        }
    };

    using WaitQueue = std::priority_queue<WaitEntry>;

    class System {
        sm::FlatHashMap<IObject*, WaitQueue> mWaitQueue;
        std::priority_queue<TimeoutEntry> mTimeoutQueue;
        sm::FlatHashMap<OsHandle, IObject*> mHandles;
    };
}
