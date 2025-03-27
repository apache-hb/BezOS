#pragma once

#include <bezos/handle.h>

#include "std/rcuptr.hpp"
#include "util/absl.hpp"

#include "std/rcu.hpp"

#include <compare> // IWYU pragma: keep
#include <queue>

namespace sys2 {
    class IObject;
    class Process;
    class Thread;
    class GlobalSchedule;

    struct SleepEntry {
        OsInstant wake;
        sm::RcuWeakPtr<Thread> thread;

        constexpr auto operator<=>(const SleepEntry& other) const noexcept {
            return wake <=> other.wake;
        }
    };

    struct WaitEntry {
        /// @brief Timeout when this entry should be completed
        OsInstant timeout;

        /// @brief The thread that is waiting
        sm::RcuWeakPtr<Thread> thread;

        /// @brief The object that is being waited on
        sm::RcuWeakPtr<IObject> object;

        constexpr auto operator<=>(const WaitEntry& other) const noexcept {
            return timeout <=> other.timeout;
        }
    };

    using WaitQueue = std::priority_queue<WaitEntry>;

    class System {
        stdx::SpinLock mLock;
        sm::RcuDomain mDomain;
        [[maybe_unused]]
        GlobalSchedule *mSchedule;

        sm::FlatHashMap<sm::RcuWeakPtr<IObject>, WaitQueue> mWaitQueue;
        std::priority_queue<WaitEntry> mTimeoutQueue;
        std::priority_queue<SleepEntry> mSleepQueue;

    public:
        System(GlobalSchedule *schedule)
            : mSchedule(schedule)
        { }

        sm::RcuDomain &rcuDomain() { return mDomain; }

        void addObject(sm::RcuWeakPtr<IObject> object);
    };
}
