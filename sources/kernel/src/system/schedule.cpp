#include "system/schedule.hpp"
#include "gdt.h"
#include "panic.hpp"
#include "system/thread.hpp"

using namespace std::chrono_literals;

sys2::CpuLocalSchedule::CpuLocalSchedule(size_t tasks)
    : mThreadStorage(new ThreadSchedulingInfo[tasks])
    , mQueue(mThreadStorage.get(), tasks)
    , mCurrent(nullptr)
{ }

static sys2::RegisterSet SaveThreadContext(km::IsrContext *context) {
    return sys2::RegisterSet {
        .rax = context->rax,
        .rbx = context->rbx,
        .rcx = context->rcx,
        .rdx = context->rdx,
        .rdi = context->rdi,
        .rsi = context->rsi,
        .r8 = context->r8,
        .r9 = context->r9,
        .r10 = context->r10,
        .r11 = context->r11,
        .r12 = context->r12,
        .r13 = context->r13,
        .r14 = context->r14,
        .r15 = context->r15,
        .rbp = context->rbp,
        .rsp = context->rsp,
        .rip = context->rip,
        .rflags = context->rflags,
    };
}

static km::IsrContext LoadThreadContext(sys2::RegisterSet& regs, bool supervisor) {
    uint64_t cs = supervisor ? (GDT_64BIT_CODE * 0x8) : ((GDT_64BIT_USER_CODE * 0x8) | 0b11);
    uint64_t ss = supervisor ? (GDT_64BIT_DATA * 0x8) : ((GDT_64BIT_USER_DATA * 0x8) | 0b11);
    return km::IsrContext {
        .rax = regs.rax,
        .rbx = regs.rbx,
        .rcx = regs.rcx,
        .rdx = regs.rdx,
        .rdi = regs.rdi,
        .rsi = regs.rsi,
        .r8 = regs.r8,
        .r9 = regs.r9,
        .r10 = regs.r10,
        .r11 = regs.r11,
        .r12 = regs.r12,
        .r13 = regs.r13,
        .r14 = regs.r14,
        .r15 = regs.r15,
        .rbp = regs.rbp,

        .vector = 0,
        .error = 0,

        .rip = regs.rip,
        .cs = cs,
        .rflags = regs.rflags,
        .rsp = regs.rsp,
        .ss = ss,
    };
}

bool sys2::CpuLocalSchedule::reschedule() {
    ThreadSchedulingInfo info;
    while (mQueue.tryPollBack(info)) {
        if (sm::RcuSharedPtr<Thread> thread = info.thread.lock()) {
            if (mCurrent != nullptr) {
                mQueue.addFront(ThreadSchedulingInfo { mCurrent });
            }

            mCurrent = thread;
            return true;
        }
    }

    return false;
}

km::IsrContext sys2::CpuLocalSchedule::serviceSchedulerInt(km::IsrContext *context) {
    auto previous = mCurrent;

    if (!reschedule()) {
        return *context;
    }

    auto oldRegs = SaveThreadContext(context);
    previous->saveState(oldRegs);

    auto current = mCurrent;

    auto newRegs = current->loadState();
    return LoadThreadContext(newRegs, current->isSupervisor());
}

sm::RcuSharedPtr<sys2::Thread> sys2::CpuLocalSchedule::currentThread() {
    return mCurrent;
}

sm::RcuSharedPtr<sys2::Process> sys2::CpuLocalSchedule::currentProcess() {
    if (mCurrent == nullptr) {
        return nullptr;
    }

    ThreadStat info;
    if (mCurrent->stat(&info) != OsStatusSuccess) {
        return nullptr;
    }

    return info.process;
}

OsStatus sys2::CpuLocalSchedule::addThread(sm::RcuSharedPtr<Thread> thread) {
    ThreadSchedulingInfo info { thread.weak() };
    return mQueue.addFront(info) ? OsStatusSuccess : OsStatusOutOfMemory;
}

sys2::GlobalSchedule::GlobalSchedule(GlobalScheduleCreateInfo info)
    : mCpuCount(info.cpus)
    , mCpuLocal(new sys2::CpuLocalSchedule[mCpuCount])
{
    for (size_t i = 0; i < mCpuCount; i++) {
        mCpuLocal[i] = CpuLocalSchedule(info.tasks);
    }
}

OsStatus sys2::GlobalSchedule::addProcess(sm::RcuSharedPtr<Process> process) {
    stdx::UniqueLock guard(mLock);
    mProcessInfo.insert({ process.weak(), ProcessSchedulingInfo {} });
    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::addThread(sm::RcuSharedPtr<Thread> thread) {
    for (size_t i = 0; i < mCpuCount; i++) {
        size_t index = (mLastScheduled++) % mCpuCount;
        if (mCpuLocal[index].addThread(thread) == OsStatusSuccess) {
            return OsStatusSuccess;
        }
    }

    return OsStatusOutOfMemory;
}

OsStatus sys2::GlobalSchedule::removeProcess(sm::RcuWeakPtr<Process> process) {
    stdx::UniqueLock guard(mLock);
    auto iter = mProcessInfo.find(process);
    if (iter == mProcessInfo.end()) {
        return OsStatusNotFound;
    }

    mProcessInfo.erase(iter);
    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::removeThread(sm::RcuWeakPtr<Thread>) {
    return OsStatusNotFound;
}

OsStatus sys2::GlobalSchedule::suspend(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadRunning;

    while (true) {
        if (thread->cmpxchgState(state, eOsThreadSuspended)) {
            return OsStatusSuccess;
        }

        switch (state) {
        case eOsThreadSuspended:
            return OsStatusSuccess;
        case eOsThreadQueued:
        case eOsThreadRunning:
        case eOsThreadWaiting:
            continue;
        case eOsThreadFinished:
        case eOsThreadOrphaned:
            return OsStatusCompleted;
        }
    }

    KM_PANIC("Thread state is invalid");
}

OsStatus sys2::GlobalSchedule::resume(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadSuspended;

    while (true) {
        if (thread->cmpxchgState(state, eOsThreadRunning)) {
            return OsStatusSuccess;
        }

        switch (state) {
        case eOsThreadSuspended:
            continue;
        case eOsThreadQueued:
        case eOsThreadRunning:
        case eOsThreadWaiting:
            return OsStatusSuccess;
        case eOsThreadFinished:
        case eOsThreadOrphaned:
            return OsStatusCompleted;
        }
    }

    KM_PANIC("Thread state is invalid");
}

OsStatus sys2::GlobalSchedule::sleep(sm::RcuSharedPtr<Thread> thread, OsInstant wake) {
    if (OsStatus status = suspend(thread)) {
        return status;
    }

    stdx::UniqueLock guard(mLock);
    mSleepQueue.push(SleepEntry {
        .wake = wake,
        .thread = thread.weak(),
    });

    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::wait(sm::RcuSharedPtr<Thread> thread, sm::RcuSharedPtr<IObject> object, OsInstant timeout) {
    if (OsStatus status = suspend(thread)) {
        return status;
    }

    stdx::UniqueLock guard(mLock);
    mWaitQueue[object].push(WaitEntry {
        .timeout = timeout,
        .thread = thread.weak(),
    });

    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::signal(sm::RcuSharedPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    auto iter = mWaitQueue.find(object);
    if (iter == mWaitQueue.end()) {
        return OsStatusNotFound;
    }

    auto& queue = iter->second;
    while (!queue.empty()) {
        auto entry = queue.top();
        queue.pop();

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resume(thread)) {
                return status;
            }
        }
    }

    mWaitQueue.erase(iter);
    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::resumeSleepQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mSleepQueue.empty()) {
        auto entry = mSleepQueue.top();
        if (entry.wake > now) {
            break;
        }

        mSleepQueue.pop();

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resume(thread)) {
                result = status;
            }
        }
    }

    return result;
}

OsStatus sys2::GlobalSchedule::resumeWaitQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mTimeoutQueue.empty()) {
        auto entry = mTimeoutQueue.top();
        if (entry.timeout > now) {
            break;
        }

        mTimeoutQueue.pop();

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resume(thread)) {
                result = status;
            }
        }
    }

    return result;
}

OsStatus sys2::GlobalSchedule::update(OsInstant now) {
    OsStatus result = OsStatusSuccess;

    stdx::UniqueLock guard(mLock);

    if (OsStatus status = resumeSleepQueue(now)) {
        result = status;
    }

    if (OsStatus status = resumeWaitQueue(now)) {
        result = status;
    }

    return result;
}
