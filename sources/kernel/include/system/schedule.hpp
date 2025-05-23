#pragma once

#include <bezos/status.h>
#include <bezos/handle.h>
#include <queue>

#include "isr/isr.hpp"

#include "processor.hpp"
#include "std/ringbuffer.hpp"
#include "std/shared_spinlock.hpp"

#include "std/rcuptr.hpp"
#include "system/access.hpp"
#include "util/absl.hpp"

namespace km {
    class ApicTimer;
}

namespace sys {
    class GlobalSchedule;
    class CpuLocalSchedule;
    class ITask;

    struct ThreadSchedulingInfo {
        sm::RcuWeakPtr<Thread> thread;
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
        sm::AtomicRingQueue<ThreadSchedulingInfo> mTaskQueue;
        sm::RcuSharedPtr<Thread> mCurrent;
        GlobalSchedule *mGlobal;

        bool startThread(sm::RcuSharedPtr<Thread> thread);
        bool stopThread(sm::RcuSharedPtr<Thread> thread);

    public:
        CpuLocalSchedule() = default;
        CpuLocalSchedule(size_t tasks, GlobalSchedule *global);

        sm::RcuSharedPtr<Thread> currentThread() noexcept [[clang::reentrant, clang::nonblocking]];
        sm::RcuSharedPtr<Process> currentProcess() noexcept [[clang::reentrant, clang::nonblocking]];

        OsStatus addThread(sm::RcuSharedPtr<Thread> thread);

        bool reschedule() noexcept [[clang::reentrant, clang::nonblocking]];

        bool scheduleNextContext(km::IsrContext *context, km::IsrContext *next, void **syscallStack);

        size_t tasks() {
            return mTaskQueue.count();
        }
    };

    class GlobalSchedule {
        stdx::SharedSpinLock mLock;
        sm::FlatHashMap<km::CpuCoreId, std::unique_ptr<CpuLocalSchedule>> mCpuLocal;

        sm::FlatHashMap<sm::RcuWeakPtr<IObject>, WaitQueue> mWaitQueue GUARDED_BY(mLock);
        std::priority_queue<WaitEntry> mTimeoutQueue GUARDED_BY(mLock);
        std::priority_queue<SleepEntry> mSleepQueue GUARDED_BY(mLock);

        sm::FlatHashSet<sm::RcuWeakPtr<Thread>> mSuspendSet GUARDED_BY(mLock);

        void wakeQueue(OsInstant now, sm::RcuWeakPtr<IObject> object) REQUIRES(mLock);

        OsStatus resumeSleepQueue(OsInstant now) REQUIRES(mLock);
        OsStatus resumeWaitQueue(OsInstant now) REQUIRES(mLock);

        OsStatus scheduleThread(sm::RcuSharedPtr<Thread> thread);

        OsStatus suspendUnlocked(sm::RcuSharedPtr<Thread> thread) REQUIRES(mLock);
        OsStatus resumeUnlocked(sm::RcuSharedPtr<Thread> thread) REQUIRES(mLock);

        void doSuspendUnlocked(sm::RcuSharedPtr<Thread> thread) [[clang::nonreentrant]] REQUIRES(mLock);

        void doResumeUnlocked(sm::RcuSharedPtr<Thread> thread) REQUIRES(mLock);
    public:
        GlobalSchedule() = default;

        GlobalSchedule(GlobalSchedule&& other)
            : mCpuLocal(std::move(other.mCpuLocal))
            , mWaitQueue(std::move(other.mWaitQueue))
            , mTimeoutQueue(std::move(other.mTimeoutQueue))
            , mSleepQueue(std::move(other.mSleepQueue))
            , mSuspendSet(std::move(other.mSuspendSet))
        { }

        OsStatus addThread(sm::RcuSharedPtr<Thread> thread);

        OsStatus removeThread(sm::RcuWeakPtr<Thread> thread);

        OsStatus suspend(sm::RcuSharedPtr<Thread> thread);
        OsStatus resume(sm::RcuSharedPtr<Thread> thread);

        OsStatus sleep(sm::RcuSharedPtr<Thread> thread, OsInstant wake);
        OsStatus wait(sm::RcuSharedPtr<Thread> thread, sm::RcuSharedPtr<IObject> object, OsInstant timeout);

        OsStatus signal(sm::RcuSharedPtr<IObject> object, OsInstant now);

        OsStatus tick(OsInstant now);

        void initCpuSchedule(km::CpuCoreId cpu, size_t tasks);

        CpuLocalSchedule *getCpuSchedule(km::CpuCoreId cpu) {
            stdx::SharedLock guard(mLock);
            if (auto iter = mCpuLocal.find(cpu); iter != mCpuLocal.end()) {
                return iter->second.get();
            }

            return nullptr;
        }

        void doSuspend(sm::RcuSharedPtr<Thread> thread);

        void doResume(sm::RcuSharedPtr<Thread> thread);
    };

    [[noreturn]]
    void EnterScheduler(CpuLocalSchedule *scheduler, km::ApicTimer *apicTimer);

    void InstallTimerIsr(km::SharedIsrTable *table);

    sm::RcuSharedPtr<Process> GetCurrentProcess();
    sm::RcuSharedPtr<Thread> GetCurrentThread();
    void YieldCurrentThread();
}
