#pragma once

#include <bezos/handle.h>

#include "util/absl.hpp"

#include <compare>
#include <queue>

namespace sys2 {
    class IObject;
    class Process;
    class Thread;

    struct SleepEntry {
        OsInstant wake;
        Thread *thread;

        constexpr auto operator<=>(const SleepEntry& other) const noexcept {
            return wake <=> other.wake;
        }
    };

    struct WaitEntry {
        /// @brief Timeout when this entry should be completed
        OsInstant timeout;

        /// @brief The thread that is waiting
        Thread *thread;

        /// @brief The object that is being waited on
        IObject *object;

        constexpr auto operator<=>(const WaitEntry& other) const noexcept {
            return timeout <=> other.timeout;
        }
    };

    using WaitQueue = std::priority_queue<WaitEntry>;

    class System {
        sm::FlatHashMap<IObject*, WaitQueue> mWaitQueue;
        std::priority_queue<WaitEntry> mTimeoutQueue;
        std::priority_queue<SleepEntry> mSleepQueue;
    };
}
