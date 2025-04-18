#include "system/schedule.hpp"
#include "panic.hpp"
#include "system/thread.hpp"
#include <bezos/facility/threads.h>

using namespace std::chrono_literals;

sys2::CpuLocalSchedule::CpuLocalSchedule(size_t tasks, GlobalSchedule *global)
    : mQueue(tasks)
    , mCurrent(nullptr)
    , mGlobal(global)
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
        .cs = context->cs,
        .ss = context->ss,
    };
}

static km::IsrContext LoadThreadContext(sys2::RegisterSet& regs) {
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
        .cs = regs.cs,
        .rflags = regs.rflags,
        .rsp = regs.rsp,
        .ss = regs.ss,
    };
}

bool sys2::CpuLocalSchedule::startThread(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState expected = eOsThreadQueued;
    while (!thread->cmpxchgState(expected, eOsThreadRunning)) {
        switch (expected) {
        case eOsThreadSuspended:
            // If the thread has been suspended or is waiting drop it from the scheduler.
            mGlobal->doSuspend(thread);
            return false;
        case eOsThreadWaiting:
            // If the thread is waiting then drop it from the scheduler.
            // Part of the wait logic already adds it to a wait queue.
            return false;
        case eOsThreadOrphaned:
        case eOsThreadFinished:
            // Thread doesnt need to be scheduled anymore.
            return false;

        case eOsThreadQueued:
            KM_PANIC("cmpxchg was not strong");
        case eOsThreadRunning:
            return true;
        default:
            continue;
        }
    }

    return true;
}

bool sys2::CpuLocalSchedule::stopThread(sm::RcuSharedPtr<Thread> thread) {
    if (thread == nullptr) {
        return false;
    }

    OsThreadState expected = eOsThreadRunning;
    while (!thread->cmpxchgState(expected, eOsThreadQueued)) {
        switch (expected) {
        case eOsThreadRunning:
        case eOsThreadQueued:
            return true;
        case eOsThreadSuspended:
        case eOsThreadWaiting:
            // Thread is already suspended or waiting.
            return false;
        case eOsThreadOrphaned:
        case eOsThreadFinished:
            // Thread is already finished or orphaned.
            return false;
        default:
            continue;
        }
    }

    return true;
}

bool sys2::CpuLocalSchedule::reschedule() {
    ThreadSchedulingInfo info;
    while (mQueue.try_dequeue(info)) {
        if (sm::RcuSharedPtr<Thread> thread = info.thread.lock()) {

            if (!startThread(thread)) {
                continue;
            }

            if (stopThread(mCurrent)) {
                mQueue.enqueue(ThreadSchedulingInfo { mCurrent });
            }

            mCurrent = thread;
            return true;
        }
    }

    if (mCurrent == nullptr) {
        return false;
    }

    if (startThread(mCurrent)) {
        return true;
    }

    if (stopThread(mCurrent)) {
        mQueue.enqueue(ThreadSchedulingInfo { mCurrent });
    }

    return false;
}

bool sys2::CpuLocalSchedule::scheduleNextContext(km::IsrContext *context, km::IsrContext *next, void **syscallStack) {
    auto oldThread = mCurrent;

    if (!reschedule()) {
        return false;
    }

    if (oldThread != nullptr) {
        auto oldRegs = SaveThreadContext(context);
        oldThread->saveState(oldRegs);
    }

    auto newThread = mCurrent;

    auto newRegs = newThread->loadState();
    *next = LoadThreadContext(newRegs);
    *syscallStack = newThread->getKernelStack().baseAddress();
    return true;
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

    OsStatus status = mQueue.try_enqueue(info) ? OsStatusSuccess : OsStatusOutOfMemory;
    return status;
}

OsStatus sys2::GlobalSchedule::scheduleThread(sm::RcuSharedPtr<Thread> thread) {
    auto it = std::min_element(mCpuLocal.begin(), mCpuLocal.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.second->tasks() < rhs.second->tasks();
        });

    if (it == mCpuLocal.end()) {
        return OsStatusOutOfMemory;
    }

    auto& [cpuId, cpu] = *it;
    if (cpu->addThread(thread) == OsStatusSuccess) {
        return OsStatusSuccess;
    }

    // If the CPU local schedule is full, we need to find another CPU
    // to add the thread to.
    for (auto& [cpuId, cpu] : mCpuLocal) {
        if (cpu->addThread(thread) == OsStatusSuccess) {
            return OsStatusSuccess;
        }
    }

    // If we reach here, it means that all CPU local schedules are full
    // and we couldn't add the thread to any of them.

    return OsStatusOutOfMemory;
}

OsStatus sys2::GlobalSchedule::addThread(sm::RcuSharedPtr<Thread> thread) {
    km::IntGuard iguard;
    stdx::SharedLock guard(mLock);
    return scheduleThread(thread);
}

void sys2::GlobalSchedule::initCpuSchedule(km::CpuCoreId cpu, size_t tasks) {
    stdx::UniqueLock guard(mLock);

    mCpuLocal.insert({ cpu, std::make_unique<CpuLocalSchedule>(tasks, this) });
}

OsStatus sys2::GlobalSchedule::removeThread(sm::RcuWeakPtr<Thread>) {
    return OsStatusNotFound;
}

OsStatus sys2::GlobalSchedule::suspend(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadQueued;

    while (!thread->cmpxchgState(state, eOsThreadSuspended)) {
        switch (state) {
        case eOsThreadSuspended:
            doSuspend(thread);
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

    doSuspend(thread);
    return OsStatusSuccess;
}

OsStatus sys2::GlobalSchedule::resume(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadSuspended;

    while (!thread->cmpxchgState(state, eOsThreadRunning)) {
        switch (state) {
        case eOsThreadSuspended:
            doResume(thread);
            return OsStatusSuccess;
        case eOsThreadQueued:
        case eOsThreadRunning:
        case eOsThreadWaiting:
            return OsStatusSuccess;
        case eOsThreadFinished:
        case eOsThreadOrphaned:
            return OsStatusCompleted;
        }
    }

    doResume(thread);
    return OsStatusSuccess;
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

OsStatus sys2::GlobalSchedule::signal(sm::RcuSharedPtr<IObject> object, OsInstant now) {
    stdx::UniqueLock guard(mLock);
    auto iter = mWaitQueue.find(object);
    if (iter == mWaitQueue.end()) {
        return OsStatusNotFound;
    }

    auto& queue = iter->second;
    while (!queue.empty()) {
        WaitEntry entry = queue.top();
        queue.pop();

        if (auto thread = entry.thread.lock()) {
            OsStatus state = (entry.timeout < now) ? OsStatusTimeout : OsStatusCompleted;
            thread->setSignalStatus(state);

            if (OsStatus status = resume(thread)) {
                return status;
            }
        }
    }

    mWaitQueue.erase(iter);
    return OsStatusSuccess;
}

void sys2::GlobalSchedule::doSuspend(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    mSuspendSet.insert(thread.weak());
}

void sys2::GlobalSchedule::doResume(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    if (auto it = mSuspendSet.find(thread.weak()); it != mSuspendSet.end()) {
        mSuspendSet.erase(thread.weak());
        scheduleThread(thread);
    }
}

OsStatus sys2::GlobalSchedule::resumeSleepQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mSleepQueue.empty()) {
        SleepEntry entry = mSleepQueue.top();
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

void sys2::GlobalSchedule::wakeQueue(OsInstant now, sm::RcuWeakPtr<IObject> object) {
    if (auto it = mWaitQueue.find(object); it != mWaitQueue.end()) {
        auto& [_, queue] = *it;
        while (!queue.empty()) {
            WaitEntry entry = queue.top();
            if (entry.timeout > now) {
                break;
            }

            queue.pop();
        }

        if (queue.empty()) {
            mWaitQueue.erase(it);
        }
    }
}

OsStatus sys2::GlobalSchedule::resumeWaitQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mTimeoutQueue.empty()) {
        WaitEntry entry = mTimeoutQueue.top();
        if (entry.timeout > now) {
            break;
        }

        mTimeoutQueue.pop();

        wakeQueue(now, entry.object);

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resume(thread)) {
                result = status;
            }
        }
    }

    return result;
}

OsStatus sys2::GlobalSchedule::tick(OsInstant now) {
    OsStatus result = OsStatusSuccess;

    stdx::UniqueLock guard(mLock);

    if (OsStatus status = resumeSleepQueue(now)) {
        result = status;
    }

    if (OsStatus status = resumeWaitQueue(now)) {
        result = status;
    }

    for (auto thread : mSuspendSet) {
        if (auto t = thread.lock()) {
            if (t->state() == eOsThreadQueued) {
                scheduleThread(t);
            }
        }
    }

    absl::erase_if(mSuspendSet, [](auto thread) {
        sm::RcuWeakPtr<Thread> ref = thread;
        if (auto t = ref.lock()) {
            auto state = t->state();
            return state == eOsThreadQueued || state == eOsThreadOrphaned || state == eOsThreadFinished;
        }

        return true;
    });

    return result;
}

// TODO: figure out a better way to allocate scheduler queues
#if __STDC_HOSTED__ == 1
void *sys2::SchedulerQueueTraits::malloc(size_t size) {
    return std::malloc(size);
}
void sys2::SchedulerQueueTraits::free(void *ptr) {
    std::free(ptr);
}
#endif
