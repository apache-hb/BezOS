#include "system/system.hpp"

#include "memory/address_space.hpp"
#include "memory/layout.hpp"
#include "memory/page_allocator.hpp"

#include "system/create.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"
#include "system/thread.hpp"

#include "xsave.hpp"

static constexpr size_t kDefaultPtePageCount = 64;
static constexpr size_t kDefaultKernelStackSize = 4;

void sys2::System::addObject(sm::RcuSharedPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
}

void sys2::System::removeObject(sm::RcuWeakPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
}

void sys2::System::addProcessObject(sm::RcuSharedPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.insert(object);
    mSchedule.addProcess(object);
}

void sys2::System::removeProcessObject(sm::RcuWeakPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.erase(object);
}

void sys2::System::addThreadObject(sm::RcuSharedPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
    mSchedule.addThread(object);
}

void sys2::System::removeThreadObject(sm::RcuWeakPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
}

OsStatus sys2::System::mapProcessPageTables(km::AddressMapping *mapping) {
    km::MemoryRange range = mPageAllocator->alloc4k(kDefaultPtePageCount);
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->map(range, km::PageFlags::eData, km::MemoryType::eWriteBack, mapping)) {
        mPageAllocator->release(range);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::System::mapSystemStack(km::StackMapping *mapping) {
    km::MemoryRange stack = mPageAllocator->alloc4k(kDefaultKernelStackSize);
    if (stack.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->mapStack(stack, km::PageFlags::eData, mapping)) {
        mPageAllocator->release(stack);
        return status;
    }

    return OsStatusSuccess;
}

void sys2::System::releaseMemory(km::MemoryRange range) {
    mPageAllocator->release(range);
}

OsStatus sys2::System::releaseMapping(km::AddressMapping mapping) {
    if (OsStatus status = mSystemTables->unmap(mapping.virtualRange())) {
        return status;
    }

    mPageAllocator->release(mapping.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys2::System::releaseStack(km::StackMapping mapping) {
    OsStatus status = mSystemTables->unmapStack(mapping);
    if (status != OsStatusSuccess) {
        return status;
    }

    mPageAllocator->release(mapping.stack.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys2::System::getProcessList(stdx::Vector2<sm::RcuSharedPtr<Process>>& list) {
    stdx::SharedLock guard(mLock);
    list.reserve(mProcessObjects.size());
    for (auto& process : mProcessObjects) {
        list.push_back(process);
    }

    return OsStatusSuccess;
}

static OsStatus CreateProcessInner(sys2::System *system, const sys2::ProcessCreateInfo& info, sm::RcuSharedPtr<sys2::Process> parent, OsHandle id, sys2::ProcessHandle **handle) {
    sm::RcuSharedPtr<sys2::Process> process;
    sys2::ProcessHandle *result = nullptr;
    km::AddressMapping pteMemory;

    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    // Create the process.
    process = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, parent, system->pageTables(), pteMemory);
    if (!process) {
        goto outOfMemory;
    }

    result = new (std::nothrow) sys2::ProcessHandle(process, id, sys2::ProcessAccess::eAll);
    if (!result) {
        goto outOfMemory;
    }

    // Add the process to the parent.
    system->addProcessObject(process);
    *handle = result;

    return OsStatusSuccess;

outOfMemory:
    delete result;
    process.reset();
    system->releaseMapping(pteMemory);
    return OsStatusOutOfMemory;
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

OsStatus sys2::SysCreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    OsHandle id = OS_HANDLE_NEW(eOsHandleProcess, 0);
    ProcessHandle *result = nullptr;

    if (OsStatus status = CreateProcessInner(system, info, nullptr, id, &result)) {
        return status;
    }

    *handle = result;
    return OsStatusSuccess;
}

// handle

OsStatus sys2::SysHandleClose(InvokeContext *, IHandle *handle) {
    delete handle;
    return OsStatusSuccess;
}

OsStatus sys2::SysHandleClone(InvokeContext *, HandleCloneInfo info, IHandle **handle) {
    ProcessHandle *target = info.process;
    IHandle *source = info.handle;
    IHandle *result = nullptr;

    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = source->clone(info.access, &result)) {
        return status;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    *handle = result;
    process->addHandle(result);

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleStat(InvokeContext *, IHandle *handle, HandleStat *result) {
    *result = HandleStat {
        .access = handle->getAccess(),
    };

    return OsStatusSuccess;
}

// process

OsStatus sys2::SysCreateProcess(InvokeContext *context, ProcessCreateInfo info, ProcessHandle **handle) {
    ProcessHandle *parent = info.process;
    TxHandle *tx = info.tx;
    ProcessHandle *result = nullptr;

    // Check that we can control the parent process.
    if (!parent->hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    // If there is a transaction, ensure we can append to it.
    if (tx && !tx->hasAccess(TxAccess::eWrite)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> invoker = context->process->getProcess();
    sm::RcuSharedPtr<Process> owner = parent->getProcess();
    OsHandle id = invoker->newHandleId(eOsHandleProcess);
    if (OsStatus status = CreateProcessInner(context->system, info, owner, id, &result)) {
        return status;
    }

    owner->addChild(result->getProcess());
    invoker->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyProcess(InvokeContext *context, ProcessDestroyInfo info) {
    sys2::ProcessHandle *handle = info.object;

    if (!handle->hasAccess(ProcessAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = handle->getProcess();
    if (OsStatus status = process->destroy(context->system, info)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysProcessStat(InvokeContext *, ProcessStatInfo info, ProcessStatResult *result) {
    sys2::ProcessHandle *handle = info.object;

    if (!handle->hasAccess(ProcessAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = handle->getProcess();
    if (OsStatus status = process->stat(result)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysQueryProcessList(InvokeContext *context, ProcessQueryInfo info, ProcessQueryResult *result) {
    sys2::System *system = context->system;
    sm::RcuSharedPtr<Process> process = context->process->getProcess();
    stdx::SharedLock guard(system->mLock);

    size_t index = 0;
    bool hasMoreData = false;
    for (sm::RcuSharedPtr<Process> child : system->mProcessObjects) {
        if (child->getName() != info.matchName) {
            continue;
        }

        if (index >= info.limit) {
            hasMoreData = true;
            break;
        }

        HandleCreateInfo createInfo {
            .owner = process,
            .access = info.access,
        };
        IHandle *handle = nullptr;
        OsStatus status = child->open(createInfo, &handle);
        if (status != OsStatusSuccess) {
            continue;
        }

        info.handles[index] = handle->getHandle();

        index += 1;
    }

    result->found = index;

    return hasMoreData ? OsStatusMoreData : OsStatusSuccess;
}

// thread

OsStatus sys2::SysCreateThread(InvokeContext *context, ThreadCreateInfo info, ThreadHandle **handle) {
    ProcessHandle *parent = info.process;

    if (!parent->hasAccess(ProcessAccess::eThreadControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();
    sm::RcuSharedPtr<Process> invoker = context->process->getProcess();
    OsHandle id = process->newHandleId(eOsHandleThread);
    ThreadHandle *result = nullptr;

    if (OsStatus status = CreateThreadInner(context->system, info, process, id, &result)) {
        return status;
    }

    process->addThread(result->getThread());
    invoker->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyThread(InvokeContext *context, ThreadDestroyInfo info) {
    ThreadHandle *handle = info.object;

    if (!handle->hasAccess(ThreadAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = handle->getThread();
    if (OsStatus status = thread->destroy(context->system, info)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysThreadStat(InvokeContext *, ThreadHandle *handle, ThreadStat *result) {
    if (!handle->hasAccess(ThreadAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Thread> thread = handle->getThread();
    if (OsStatus status = thread->stat(result)) {
        return status;
    }

    return OsStatusSuccess;
}

// tx

OsStatus sys2::SysCreateTx(InvokeContext *invoke, TxCreateInfo createInfo, TxHandle **handle) {
    sys2::ProcessHandle *parent = createInfo.process;
    sys2::ProcessHandle *invoker = invoke->process;

    if (!parent->hasAccess(ProcessAccess::eTxControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Tx> tx = sm::rcuMakeShared<Tx>(&invoke->system->rcuDomain(), createInfo);
    if (!tx) {
        return OsStatusOutOfMemory;
    }

    OsHandle id = invoker->getProcess()->newHandleId(eOsHandleTx);
    TxHandle *result = new (std::nothrow) TxHandle(tx, id, TxAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    invoker->getProcess()->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysCommitTx(InvokeContext *, TxDestroyInfo) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysAbortTx(InvokeContext *, TxDestroyInfo) {
    return OsStatusNotSupported;
}

// mutex

OsStatus sys2::SysCreateMutex(InvokeContext *, MutexCreateInfo, MutexHandle **) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysDestroyMutex(InvokeContext *, MutexDestroyInfo) {
    return OsStatusNotSupported;
}
