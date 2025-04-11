#include "system/schedule.hpp"
#include "gdt.h"
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
