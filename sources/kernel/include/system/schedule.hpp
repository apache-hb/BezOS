#pragma once

#include "isr/isr.hpp"

#include "std/fixed_deque.hpp"

#include "std/rcuptr.hpp"

namespace sys2 {
    class Process;
    class Thread;

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

    class GlobalSchedule {
        size_t mCpuCount;
        size_t mLastScheduled;
        std::unique_ptr<CpuLocalSchedule[]> mCpuLocal;
        sm::FlatHashMap<sm::RcuWeakPtr<Process>, ProcessSchedulingInfo> mProcessInfo;

    public:
        GlobalSchedule(size_t cpus, size_t tasks);

        OsStatus addProcess(sm::RcuSharedPtr<Process> process);
        OsStatus addThread(sm::RcuSharedPtr<Thread> thread);
    };
}
