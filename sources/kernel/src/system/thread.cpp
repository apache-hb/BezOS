#include "system/thread.hpp"

#include "system/process.hpp"
#include "system/system.hpp"

#include "task/scheduler.hpp"
#include "thread.hpp"
#include "xsave.hpp"
#include "gdt.h"

#include <bezos/handle.h>

static sys::RegisterSet MakeRegisterSet(OsMachineContext machine, bool supervisor) {
    uint64_t cs = supervisor ? (GDT_64BIT_CODE * 0x8) : ((GDT_64BIT_USER_CODE * 0x8) | 0b11);
    uint64_t ss = supervisor ? (GDT_64BIT_DATA * 0x8) : ((GDT_64BIT_USER_DATA * 0x8) | 0b11);

    return sys::RegisterSet {
        .rax = machine.rax,
        .rbx = machine.rbx,
        .rcx = machine.rcx,
        .rdx = machine.rdx,
        .rdi = machine.rdi,
        .rsi = machine.rsi,
        .r8  = machine.r8,
        .r9  = machine.r9,
        .r10 = machine.r10,
        .r11 = machine.r11,
        .r12 = machine.r12,
        .r13 = machine.r13,
        .r14 = machine.r14,
        .r15 = machine.r15,
        .rbp = machine.rbp,
        .rsp = machine.rsp,
        .rip = machine.rip,
        .rflags = 0x202,
        .cs = cs,
        .ss = ss,
    };
}

sys::ThreadHandle::ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access)
    : BaseHandle(thread, handle, access)
{ }

OsStatus sys::Thread::stat(ThreadStat *info) {
    sm::RcuSharedPtr<Process> process = mProcess.lock();
    if (!process) {
        return OsStatusProcessOrphaned;
    }

    stdx::SharedLock guard(mLock);

    *info = ThreadStat {
        .name = getNameUnlocked(),
        .process = process,
        .state = mThreadState,
    };

    return OsStatusSuccess;
}

void sys::Thread::saveState(RegisterSet& regs) {
    // mCpuState = regs;
    // mTlsAddress = IA32_FS_BASE.load();
    // if (mFpuState) {
    //     km::XSaveStoreState(mFpuState.get());
    // }
}

sys::RegisterSet sys::Thread::loadState() {
    // IA32_FS_BASE = mTlsAddress;
    // if (mFpuState) {
    //     km::XSaveLoadState(mFpuState.get());
    // }

    if (auto process = mProcess.lock()) {
        process->loadPageTables();
    }

    return mCpuState;
}

bool sys::Thread::isSupervisor() {
    if (auto parent = mProcess.lock()) {
        return parent->isSupervisor();
    }

    return false;
}

void sys::Thread::setSignalStatus(OsStatus status) {
    mCpuState.rax = status;
}

sys::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack)
    : Thread(createInfo, process, sys::XSaveState{fpuState, &km::DestroyXSave}, kernelStack)
{ }

sys::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, sys::XSaveState fpuState, km::StackMapping kernelStack)
    : Super(createInfo.name)
    , mCpuState(createInfo.cpuState)
    , mFpuState(std::move(fpuState))
    , mTlsAddress(createInfo.tlsAddress)
    , mThreadState(createInfo.state)
{
    mProcess = process;
    mKernelStack = kernelStack;

    if (mThreadState == eOsThreadRunning || mThreadState == eOsThreadQueued) {
        mThreadState = eOsThreadQueued;
    } else {
        mThreadState = eOsThreadSuspended;
    }
}

sys::Thread::Thread(OsThreadCreateInfo createInfo, sm::RcuWeakPtr<Process> process, sys::XSaveState fpuState, km::StackMapping kernelStack)
    : Super(createInfo.Name)
    , mCpuState(MakeRegisterSet(createInfo.CpuState, isSupervisor()))
    , mFpuState(std::move(fpuState))
    , mTlsAddress(createInfo.TlsAddress)
    , mThreadState(createInfo.Flags)
{
    mProcess = process;
    mKernelStack = kernelStack;

    if (mThreadState == eOsThreadRunning || mThreadState == eOsThreadQueued) {
        mThreadState = eOsThreadQueued;
    } else {
        mThreadState = eOsThreadSuspended;
    }
}

OsStatus sys::Thread::suspend() {
    TaskLog.dbgf("Suspending thread ", mProcess.lock()->getName(), ":", getName());
    OsThreadState expected = eOsThreadQueued;
    while (!cmpxchgState(expected, eOsThreadSuspended)) {
        switch (expected) {
        case eOsThreadSuspended:
            return OsStatusSuccess;
        case eOsThreadWaiting:
        case eOsThreadRunning:
            continue;
        case eOsThreadFinished:
        case eOsThreadOrphaned:
            return OsStatusCompleted;
        }
    }

    return OsStatusSuccess;
}

OsStatus sys::Thread::resume() {
    TaskLog.dbgf("Resuming thread ", mProcess.lock()->getName(), ":", getName());
    OsThreadState expected = eOsThreadSuspended;
    while (!cmpxchgState(expected, eOsThreadQueued)) {
        switch (expected) {
        case eOsThreadQueued:
        case eOsThreadWaiting:
        case eOsThreadRunning:
            return OsStatusSuccess;
        case eOsThreadFinished:
        case eOsThreadOrphaned:
            return OsStatusCompleted;
        }
    }

    return OsStatusSuccess;
}

OsStatus sys::Thread::attach(task::Scheduler *scheduler) {
    task::TaskState state {
        .registers = mCpuState,
        .xsave = mFpuState.get(),
        .tlsBase = mTlsAddress,
    };

    return scheduler->enqueue(state, this);
}

km::PhysicalAddress sys::Thread::getPageMap() {
    return mProcess.lock()->getPageMap();
}

OsStatus sys::Thread::destroy(System *, OsThreadState reason) {
#if 0
    if (!mKernelStack.stack.virtualRangeEx().contains(__builtin_frame_address(0))) {
        // TODO: this leaks memory, but we can't free the kernel stack if we are actually using it.
        if (OsStatus status = system->releaseStack(mKernelStack)) {
            return status;
        }
    }
#endif

    terminate();

    if (auto process = mProcess.lock()) {
        process->removeThread(loanShared());
    }

    mThreadState = reason;

    return OsStatusSuccess;
}

sys::XSaveState sys::NewXSaveState() {
    return sys::XSaveState { km::CreateXSave(), &km::DestroyXSave };
}

static OsStatus CreateThreadInner(sys::System *system, const auto& info, sm::RcuSharedPtr<sys::Process> process, sm::RcuSharedPtr<sys::Process> parent, OsHandle id, sys::ThreadHandle **handle) {
    km::StackMapping kernelStack{};
    sys::XSaveState fpuState{nullptr, &km::DestroyXSave};
    sys::ThreadHandle *result = nullptr;
    sm::RcuSharedPtr<sys::Thread> thread;
    OsStatus status = OsStatusSuccess;

    if ((status = system->mapSystemStack(&kernelStack))) {
        return status;
    }

    fpuState = sys::NewXSaveState();
    if (!fpuState) {
        goto outOfMemory;
    }

    thread = sm::rcuMakeShared<sys::Thread>(&system->rcuDomain(), info, parent, std::move(fpuState), kernelStack);
    if (!thread) {
        goto outOfMemory;
    }

    result = new (std::nothrow) sys::ThreadHandle(thread, id, sys::ThreadAccess::eAll);
    if (!result) {
        goto outOfMemory;
    }

    if ((status = system->addThreadObject(thread))) {
        goto error;
    }

    parent->addThread(thread);
    process->addHandle(result);

    *handle = result;
    return OsStatusSuccess;

outOfMemory:
    status = OsStatusOutOfMemory;

error:
    delete result;
    thread.reset();
    fpuState.reset();
    system->releaseStack(kernelStack);
    return status;
}

OsStatus sys::SysThreadCreate(InvokeContext *context, ThreadCreateInfo info, OsThreadHandle *handle) {
    OsHandle id = context->process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, context->process, context->process, id, &result)) {
        return status;
    }

    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysThreadCreate(InvokeContext *context, OsThreadCreateInfo info, OsThreadHandle *handle) {
    sm::RcuSharedPtr<Process> process;
    if (info.Process != OS_HANDLE_INVALID) {
        ProcessHandle *hProcess = nullptr;
        if (OsStatus status = SysFindHandle(context, info.Process, &hProcess)) {
            return status;
        }

        process = hProcess->getProcess();
    } else {
        process = context->process;
    }

    OsHandle id = context->process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, context->process, process, id, &result)) {
        return status;
    }

    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysThreadDestroy(InvokeContext *context, OsThreadState reason, OsThreadHandle handle) {
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &hThread)) {
        return status;
    }

    if (!hThread->hasAccess(ThreadAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = hThread->getThread();
    if (OsStatus status = thread->destroy(context->system, reason)) {
        return status;
    }

    context->system->removeThreadObject(thread);

    return OsStatusSuccess;
}

OsStatus sys::SysThreadStat(InvokeContext *context, OsThreadHandle handle, OsThreadInfo *result) {
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &hThread)) {
        return status;
    }

    if (!hThread->hasAccess(ThreadAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = hThread->getThread();
    ThreadStat info;
    if (OsStatus status = thread->stat(&info)) {
        return status;
    }

    OsProcessHandle parent = OS_HANDLE_INVALID;
    if (OsStatus status = SysResolveObject(context, info.process, eOsProcessAccessStat, &parent)) {
        return status;
    }

    OsThreadInfo stat {
        .State = info.state,
        .Process = parent,
    };

    size_t size = std::min(sizeof(stat.Name), info.name.count());
    std::memcpy(stat.Name, info.name.data(), size);

    *result = stat;

    return OsStatusSuccess;
}

OsStatus sys::SysThreadSuspend(InvokeContext *context, OsThreadHandle handle, bool suspend) {
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &hThread)) {
        return status;
    }

    if (!hThread->hasAccess(ThreadAccess::eSuspend)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = hThread->getThread();
    if (suspend) {
        return thread->suspend();
    } else {
        return thread->resume();
    }
}
