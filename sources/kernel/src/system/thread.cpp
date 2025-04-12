#include "system/thread.hpp"
#include "system/process.hpp"

#include "system/system.hpp"
#include "thread.hpp"
#include "xsave.hpp"

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
{ }

OsStatus sys2::Thread::destroy(System *system, OsThreadState reason) {
    if (OsStatus status = system->releaseStack(mKernelStack)) {
        return status;
    }

    if (auto process = mProcess.lock()) {
        process->removeThread(loanShared());
    }

    mThreadState = reason;

    system->removeObject(loanWeak());
    return OsStatusSuccess;
}

sys2::XSaveState sys2::NewXSaveState() {
    return sys2::XSaveState { km::CreateXSave(), &km::DestroyXSave };
}

static OsStatus CreateThreadInner(sys2::System *system, const sys2::ThreadCreateInfo& info, sm::RcuWeakPtr<sys2::Process> parent, OsHandle id, sys2::ThreadHandle **handle) {
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

    *handle = result;
    system->addObject(thread);
    return OsStatusSuccess;

outOfMemory:
    delete result;
    thread.reset();
    fpuState.reset();
    system->releaseStack(kernelStack);
    return OsStatusOutOfMemory;
}

OsStatus sys2::SysCreateThread(InvokeContext *context, ThreadCreateInfo info, OsThreadHandle *handle) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eThreadControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();
    OsHandle id = process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, process, id, &result)) {
        return status;
    }

    process->addThread(result->getThread());
    process->addHandle(result);
    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyThread(InvokeContext *context, OsThreadState reason, OsThreadHandle handle) {
    ProcessHandle *parent = context->process;
    if (!parent->hasAccess(ProcessAccess::eThreadControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    ThreadHandle *hThread = nullptr;
    if (OsStatus status = process->findHandle(handle, &hThread)) {
        return status;
    }

    if (!hThread->hasAccess(ThreadAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = hThread->getThread();
    if (OsStatus status = thread->destroy(context->system, reason)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysThreadStat(InvokeContext *context, OsThreadHandle handle, OsThreadInfo *result) {
    ProcessHandle *parent = context->process;
    if (!parent->hasAccess(ProcessAccess::eThreadControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();
    ThreadHandle *hThread = nullptr;
    if (OsStatus status = process->findHandle(handle, &hThread)) {
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

    OsThreadInfo stat {
        .State = info.state,
    };

    size_t size = std::min(sizeof(stat.Name), info.name.count());
    std::memcpy(stat.Name, info.name.data(), size);

    return OsStatusSuccess;
}
