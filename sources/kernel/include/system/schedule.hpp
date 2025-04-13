#pragma once

#include <bezos/status.h>
#include <bezos/handle.h>
#include <queue>

#include "isr/isr.hpp"

#include "std/fixed_deque.hpp"
#include "std/spinlock.hpp"

#include "std/rcuptr.hpp"
#include "system/access.hpp"

namespace km {
    class ApicTimer;
}

namespace sys2 {
    struct AffinityMask {
        uint64_t mask[4] = { [0 ... 3] = UINT64_MAX }; // TODO: support more than 256 processors
    };

    struct ProcessSchedulingInfo {
        AffinityMask affinity;
    };

    struct ThreadSchedulingInfo {
        sm::RcuWeakPtr<Thread> thread;
        AffinityMask affinity;
    };

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

    struct SystemStats {
        size_t objects;
        size_t processes;
    };

    class CpuLocalSchedule {
        stdx::SpinLock mLock;
        std::unique_ptr<ThreadSchedulingInfo[]> mThreadStorage;
        stdx::FixedSizeDeque<ThreadSchedulingInfo> mQueue;
        sm::RcuSharedPtr<Thread> mCurrent;

    public:
        CpuLocalSchedule() = default;
        CpuLocalSchedule(size_t tasks);

        CpuLocalSchedule& operator=(CpuLocalSchedule&& other) {
            mThreadStorage = std::move(other.mThreadStorage);
            mQueue = std::move(other.mQueue);
            mCurrent = std::move(other.mCurrent);
            return *this;
        }

        sm::RcuSharedPtr<Thread> currentThread();
        sm::RcuSharedPtr<Process> currentProcess();

        OsStatus addThread(sm::RcuSharedPtr<Thread> thread);

        bool reschedule();

        km::IsrContext serviceSchedulerInt(km::IsrContext *context);

        size_t tasks() const { return mQueue.count(); }
    };

    struct GlobalScheduleCreateInfo {
        size_t cpus;
        size_t tasks;
    };

    class GlobalSchedule {
        stdx::SharedSpinLock mLock;
        size_t mCpuCount;
        size_t mLastScheduled;
        std::unique_ptr<CpuLocalSchedule[]> mCpuLocal;

        sm::FlatHashMap<sm::RcuWeakPtr<Process>, ProcessSchedulingInfo> mProcessInfo GUARDED_BY(mLock);

        sm::FlatHashMap<sm::RcuWeakPtr<IObject>, WaitQueue> mWaitQueue GUARDED_BY(mLock);
        std::priority_queue<WaitEntry> mTimeoutQueue GUARDED_BY(mLock);
        std::priority_queue<SleepEntry> mSleepQueue GUARDED_BY(mLock);

        OsStatus resumeSleepQueue(OsInstant now) REQUIRES(mLock);
        OsStatus resumeWaitQueue(OsInstant now) REQUIRES(mLock);

    public:
        GlobalSchedule(GlobalScheduleCreateInfo info);

        OsStatus addProcess(sm::RcuSharedPtr<Process> process);
        OsStatus addThread(sm::RcuSharedPtr<Thread> thread);

        OsStatus removeProcess(sm::RcuWeakPtr<Process> process);
        OsStatus removeThread(sm::RcuWeakPtr<Thread> thread);

        OsStatus suspend(sm::RcuSharedPtr<Thread> thread);
        OsStatus resume(sm::RcuSharedPtr<Thread> thread);

        OsStatus sleep(sm::RcuSharedPtr<Thread> thread, OsInstant wake);
        OsStatus wait(sm::RcuSharedPtr<Thread> thread, sm::RcuSharedPtr<IObject> object, OsInstant timeout);

        OsStatus signal(sm::RcuSharedPtr<IObject> object);

        OsStatus update(OsInstant now);
    };

    [[noreturn]]
    void EnterScheduler(km::LocalIsrTable *table, CpuLocalSchedule *scheduler, km::ApicTimer *apicTimer);

    sm::RcuSharedPtr<Process> GetCurrentProcess();
}
