#include "system/schedule.hpp"

#include "log.hpp"
#include "panic.hpp"
#include "std/spinlock.hpp"
#include "system/thread.hpp"

#include <bezos/facility/thread.h>

using namespace std::chrono_literals;

sys::CpuLocalSchedule::CpuLocalSchedule(size_t tasks, GlobalSchedule *global)
    : mQueue(tasks)
    , mCurrent(nullptr)
    , mGlobal(global)
{ }

static sys::RegisterSet SaveThreadContext(km::IsrContext *context) {
    return sys::RegisterSet {
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

static km::IsrContext LoadThreadContext(sys::RegisterSet& regs) {
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

bool sys::CpuLocalSchedule::startThread(sm::RcuSharedPtr<Thread> thread) {
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
        case eOsThreadRunning:
            return true;
        default:
            continue;
        }
    }

    return true;
}

bool sys::CpuLocalSchedule::stopThread(sm::RcuSharedPtr<Thread> thread) {
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

bool sys::CpuLocalSchedule::reschedule() {
    ThreadSchedulingInfo info;
    while (mQueue.try_dequeue(info)) {
        if (sm::RcuSharedPtr<Thread> thread = info.thread.lock()) {
            if (!startThread(thread)) {
                continue;
            }

            if (mCurrent) {
                stopThread(mCurrent);
                KM_ASSERT(mQueue.enqueue(ThreadSchedulingInfo { mCurrent }));
            }

            mCurrent = thread;
            break;
        }
    }

    if (mCurrent == nullptr) {
        return false;
    }

    if (startThread(mCurrent)) {
        // If the thread is already running then we dont need to
        // reschedule it.
        return true;
    }

    stopThread(mCurrent);
    KM_ASSERT(mQueue.enqueue(ThreadSchedulingInfo { mCurrent }));

    mCurrent = nullptr;

    return false;
}

bool sys::CpuLocalSchedule::scheduleNextContext(km::IsrContext *context, km::IsrContext *next, void **syscallStack) {
    auto oldThread = mCurrent;

    if (!reschedule()) {
        return false;
    }

    auto newThread = mCurrent;

    if (newThread == oldThread && newThread != nullptr) {
        *next = *context;
        *syscallStack = newThread->getKernelStack().baseAddress();
        return true;
    }

    if (oldThread != nullptr) {
        auto oldRegs = SaveThreadContext(context);
        oldThread->saveState(oldRegs);
    }

#if 0
    if (newThread == nullptr && oldThread != nullptr) {
        KmDebugMessageUnlocked("[SCHED] Dropping thread ", oldThread->getName(), " - ", km::GetCurrentCoreId(), "\n");
    } else if (newThread != nullptr && oldThread == nullptr) {
        KmDebugMessageUnlocked("[SCHED] Starting thread ", newThread->getName(), " - ", km::GetCurrentCoreId(), "\n");
    } else if (newThread != nullptr && oldThread != nullptr) {
        KmDebugMessageUnlocked("[SCHED] Switching from ", oldThread->getName(), " to ", newThread->getName(), " - ", km::GetCurrentCoreId(), "\n");
    }
#endif

    auto newRegs = newThread->loadState();
    *next = LoadThreadContext(newRegs);
    *syscallStack = newThread->getKernelStack().baseAddress();
    return true;
}

sm::RcuSharedPtr<sys::Thread> sys::CpuLocalSchedule::currentThread() {
    return mCurrent;
}

sm::RcuSharedPtr<sys::Process> sys::CpuLocalSchedule::currentProcess() {
    if (mCurrent == nullptr) {
        return nullptr;
    }

    ThreadStat info;
    if (mCurrent->stat(&info) != OsStatusSuccess) {
        return nullptr;
    }

    return info.process;
}

OsStatus sys::CpuLocalSchedule::addThread(sm::RcuSharedPtr<Thread> thread) {
    ThreadSchedulingInfo info { thread.weak() };

    OsStatus status = mQueue.try_enqueue(info) ? OsStatusSuccess : OsStatusOutOfMemory;
    return status;
}

OsStatus sys::GlobalSchedule::scheduleThread(sm::RcuSharedPtr<Thread> thread) {
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

OsStatus sys::GlobalSchedule::addThread(sm::RcuSharedPtr<Thread> thread) {
    // Note about saftey of this function:
    // We access mCpuLocal and mSuspendSet without holding the lock.
    // This is safe so long as these are not modified. They should only be modified
    // during initialization, if we ever support hotplug then we need to revisit this.
    return scheduleThread(thread);
}

void sys::GlobalSchedule::initCpuSchedule(km::CpuCoreId cpu, size_t tasks) {
    stdx::UniqueLock guard(mLock);

    mCpuLocal.insert({ cpu, std::make_unique<CpuLocalSchedule>(tasks, this) });
}

OsStatus sys::GlobalSchedule::removeThread(sm::RcuWeakPtr<Thread>) {
    return OsStatusNotFound;
}

OsStatus sys::GlobalSchedule::suspendUnlocked(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadQueued;

    while (!thread->cmpxchgState(state, eOsThreadSuspended)) {
        switch (state) {
        case eOsThreadSuspended:
            doSuspendUnlocked(thread);
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

    doSuspendUnlocked(thread);
    return OsStatusSuccess;
}

OsStatus sys::GlobalSchedule::resumeUnlocked(sm::RcuSharedPtr<Thread> thread) {
    OsThreadState state = eOsThreadSuspended;

    while (!thread->cmpxchgState(state, eOsThreadRunning)) {
        switch (state) {
        case eOsThreadSuspended:
            doResumeUnlocked(thread);
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

    doResumeUnlocked(thread);
    return OsStatusSuccess;
}

OsStatus sys::GlobalSchedule::suspend(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    return suspendUnlocked(thread);
}

OsStatus sys::GlobalSchedule::resume(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    return resumeUnlocked(thread);
}

OsStatus sys::GlobalSchedule::sleep(sm::RcuSharedPtr<Thread> thread, OsInstant wake) {
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

OsStatus sys::GlobalSchedule::wait(sm::RcuSharedPtr<Thread> thread, sm::RcuSharedPtr<IObject> object, OsInstant timeout) {
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

OsStatus sys::GlobalSchedule::signal(sm::RcuSharedPtr<IObject> object, OsInstant now) {
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

void sys::GlobalSchedule::doSuspendUnlocked(sm::RcuSharedPtr<Thread> thread) {
    mSuspendSet.insert(thread.weak());
}

void sys::GlobalSchedule::doResumeUnlocked(sm::RcuSharedPtr<Thread> thread) {
    if (auto it = mSuspendSet.find(thread.weak()); it != mSuspendSet.end()) {
        mSuspendSet.erase(it);
        scheduleThread(thread);
    }
}

void sys::GlobalSchedule::doSuspend(sm::RcuSharedPtr<Thread> thread) {
    stdx::LockGuard guard(mLock);
    doSuspendUnlocked(thread);
}

void sys::GlobalSchedule::doResume(sm::RcuSharedPtr<Thread> thread) {
    stdx::LockGuard guard(mLock);
    doResumeUnlocked(thread);
}

OsStatus sys::GlobalSchedule::resumeSleepQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mSleepQueue.empty()) {
        SleepEntry entry = mSleepQueue.top();
        if (entry.wake > now) {
            break;
        }

        mSleepQueue.pop();

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resumeUnlocked(thread)) {
                result = status;
            }
        }
    }

    return result;
}

void sys::GlobalSchedule::wakeQueue(OsInstant now, sm::RcuWeakPtr<IObject> object) {
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

OsStatus sys::GlobalSchedule::resumeWaitQueue(OsInstant now) {
    OsStatus result = OsStatusSuccess;
    while (!mTimeoutQueue.empty()) {
        WaitEntry entry = mTimeoutQueue.top();
        if (entry.timeout > now) {
            break;
        }

        mTimeoutQueue.pop();

        wakeQueue(now, entry.object);

        if (auto thread = entry.thread.lock()) {
            if (OsStatus status = resumeUnlocked(thread)) {
                result = status;
            }
        }
    }

    return result;
}

OsStatus sys::GlobalSchedule::tick(OsInstant now) {
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
void *sys::SchedulerQueueTraits::malloc(size_t size) {
    return std::malloc(size);
}
void sys::SchedulerQueueTraits::free(void *ptr) {
    std::free(ptr);
}
#endif
