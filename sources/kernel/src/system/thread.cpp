#include "system/thread.hpp"
#include "system/process.hpp"

#include "system/system.hpp"
#include "thread.hpp"
#include "xsave.hpp"

static sys2::RegisterSet MakeRegisterSet(OsMachineContext machine) {
    return sys2::RegisterSet {
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
    };
}

sys2::ThreadHandle::ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access)
    : BaseHandle(thread, handle, access)
{ }

OsStatus sys2::Thread::stat(ThreadStat *info) {
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

void sys2::Thread::saveState(RegisterSet& regs) {
    mCpuState = regs;
    mTlsAddress = IA32_FS_BASE.load();
    if (mFpuState) {
        km::XSaveStoreState(mFpuState.get());
    }
}

sys2::RegisterSet sys2::Thread::loadState() {
    IA32_FS_BASE = mTlsAddress;
    if (mFpuState) {
        km::XSaveLoadState(mFpuState.get());
    }

    return mCpuState;
}

bool sys2::Thread::isSupervisor() {
    if (auto parent = mProcess.lock()) {
        return parent->isSupervisor();
    }

    return false;
}

sys2::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack)
    : Thread(createInfo, process, sys2::XSaveState{fpuState, &km::DestroyXSave}, kernelStack)
{ }

sys2::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, sys2::XSaveState fpuState, km::StackMapping kernelStack)
    : Super(createInfo.name)
    , mProcess(process)
    , mCpuState(createInfo.cpuState)
    , mFpuState(std::move(fpuState))
    , mTlsAddress(createInfo.tlsAddress)
    , mKernelStack(kernelStack)
    , mThreadState(createInfo.state)
{
    if (mThreadState == eOsThreadRunning || mThreadState == eOsThreadQueued) {
        mThreadState = eOsThreadQueued;
    } else {
        mThreadState = eOsThreadSuspended;
    }
}

sys2::Thread::Thread(OsThreadCreateInfo createInfo, sm::RcuWeakPtr<Process> process, sys2::XSaveState fpuState, km::StackMapping kernelStack)
    : Super(createInfo.Name)
    , mProcess(process)
    , mCpuState(MakeRegisterSet(createInfo.CpuState))
    , mFpuState(std::move(fpuState))
    , mTlsAddress(createInfo.CpuState.fs)
    , mKernelStack(kernelStack)
    , mThreadState(createInfo.Flags)
{
    if (mThreadState == eOsThreadRunning || mThreadState == eOsThreadQueued) {
        mThreadState = eOsThreadQueued;
    } else {
        mThreadState = eOsThreadSuspended;
    }
}

OsStatus sys2::Thread::destroy(System *system, OsThreadState reason) {
    if (OsStatus status = system->releaseStack(mKernelStack)) {
        return status;
    }

    if (auto process = mProcess.lock()) {
        process->removeThread(loanShared());
    }

    mThreadState = reason;

    return OsStatusSuccess;
}

sys2::XSaveState sys2::NewXSaveState() {
    return sys2::XSaveState { km::CreateXSave(), &km::DestroyXSave };
}

static OsStatus CreateThreadInner(sys2::System *system, const auto& info, sm::RcuSharedPtr<sys2::Process> parent, OsHandle id, sys2::ThreadHandle **handle) {
    km::StackMapping kernelStack{};
    sys2::XSaveState fpuState{nullptr, &km::DestroyXSave};
    sys2::ThreadHandle *result = nullptr;
    sm::RcuSharedPtr<sys2::Thread> thread;

    if (OsStatus status = system->mapSystemStack(&kernelStack)) {
        return status;
    }

    fpuState = sys2::NewXSaveState();
    if (!fpuState) {
        goto outOfMemory;
    }

    thread = sm::rcuMakeShared<sys2::Thread>(&system->rcuDomain(), info, parent, std::move(fpuState), kernelStack);
    if (!thread) {
        goto outOfMemory;
    }

    result = new (std::nothrow) sys2::ThreadHandle(thread, id, sys2::ThreadAccess::eAll);
    if (!result) {
        goto outOfMemory;
    }

    system->addThreadObject(thread);
    parent->addThread(thread);
    parent->addHandle(result);

    *handle = result;
    return OsStatusSuccess;

outOfMemory:
    delete result;
    thread.reset();
    fpuState.reset();
    system->releaseStack(kernelStack);
    return OsStatusOutOfMemory;
}

OsStatus sys2::SysCreateThread(InvokeContext *context, ThreadCreateInfo info, OsThreadHandle *handle) {
    OsHandle id = context->process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, context->process, id, &result)) {
        return status;
    }

    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysCreateThread(InvokeContext *context, OsThreadCreateInfo info, OsThreadHandle *handle) {
    OsHandle id = context->process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, context->process, id, &result)) {
        return status;
    }

    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyThread(InvokeContext *context, OsThreadState reason, OsThreadHandle handle) {
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hThread)) {
        return status;
    }

    if (!hThread->hasAccess(ThreadAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = hThread->getThread();
    if (OsStatus status = thread->destroy(context->system, reason)) {
        return status;
    }

    context->system->removeThreadObject(thread);

    return OsStatusSuccess;
}

OsStatus sys2::SysThreadStat(InvokeContext *context, OsThreadHandle handle, OsThreadInfo *result) {
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hThread)) {
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
