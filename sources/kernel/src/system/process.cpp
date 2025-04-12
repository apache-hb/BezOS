#include "system/process.hpp"
#include "memory.hpp"
#include "system/system.hpp"

#include <bezos/handle.h>

OsStatus sys2::ProcessHandle::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    if (!hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    return getInner()->createProcess(system, info, handle);
}

OsStatus sys2::ProcessHandle::destroyProcess(System *system, const ProcessDestroyInfo& info) {
    if (!hasAccess(ProcessAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    return getInner()->destroy(system, info);
}

OsStatus sys2::ProcessHandle::createTx(System *system, TxCreateInfo info, TxHandle **handle) {
    if (!hasAccess(ProcessAccess::eTxControl)) {
        return OsStatusAccessDenied;
    }

    return getInner()->createTx(system, info, handle);
}

sys2::Process::Process(const ProcessCreateInfo& createInfo, sm::RcuWeakPtr<Process> parent, const km::AddressSpace *systemTables, km::AddressMapping pteMemory)
    : Super(createInfo.name)
    , mSupervisor(createInfo.supervisor)
    , mState(createInfo.state)
    , mExitCode(0)
    , mParent(parent)
    , mPteMemory(pteMemory)
    , mPageTables(systemTables, mPteMemory, km::PageFlags::eUserAll, km::DefaultUserArea())
{ }

OsStatus sys2::Process::stat(ProcessStat *info) {
    sm::RcuSharedPtr<Process> parent = mParent.lock();
    if (!parent) {
        return OsStatusProcessOrphaned;
    }

    stdx::SharedLock guard(mLock);

    OsProcessStateFlags state = mState;
    if (mSupervisor) {
        state |= eOsProcessSupervisor;
    }

    *info = ProcessStat {
        .name = getNameUnlocked(),
        .id = OsProcessId(mId),
        .exitCode = mExitCode,
        .state = state,
        .parent = parent,
    };

    return OsStatusSuccess;
}

OsStatus sys2::Process::open(HandleCreateInfo createInfo, IHandle **handle) {
    sm::RcuSharedPtr<Process> owner = createInfo.owner;
    OsHandle id = owner->newHandleId(eOsHandleProcess);
    ProcessHandle *result = new (std::nothrow) ProcessHandle(loanShared(), id, ProcessAccess(createInfo.access));
    if (!result) {
        return OsStatusOutOfMemory;
    }

    owner->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

sys2::IHandle *sys2::Process::getHandle(OsHandle handle) {
    stdx::SharedLock guard(mLock);
    if (auto it = mHandles.find(handle); it != mHandles.end()) {
        return it->second.get();
    }

    return nullptr;
}

void sys2::Process::addHandle(IHandle *handle) {
    stdx::UniqueLock guard(mLock);
    mHandles.insert({ handle->getHandle(), std::unique_ptr<IHandle>(handle) });
}

OsStatus sys2::Process::removeHandle(IHandle *handle) {
    return removeHandle(handle->getHandle());
}

OsStatus sys2::Process::removeHandle(OsHandle handle) {
    stdx::UniqueLock guard(mLock);
    if (auto it = mHandles.find(handle); it != mHandles.end()) {
        mHandles.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusInvalidHandle;
}

OsStatus sys2::Process::findHandle(OsHandle handle, OsHandleType type, IHandle **result) {
    if (OS_HANDLE_TYPE(handle) != type) {
        return OsStatusInvalidHandle;
    }

    stdx::SharedLock guard(mLock);
    if (auto it = mHandles.find(handle); it != mHandles.end()) {
        *result = it->second.get();
        return OsStatusSuccess;
    }

    return OsStatusInvalidHandle;
}

void sys2::Process::removeChild(sm::RcuSharedPtr<Process> child) {
    stdx::UniqueLock guard(mLock);
    mChildren.erase(child);
}

void sys2::Process::addChild(sm::RcuSharedPtr<Process> child) {
    stdx::UniqueLock guard(mLock);
    mChildren.insert(child);
}

void sys2::Process::removeThread(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    mThreads.erase(thread);
}

void sys2::Process::addThread(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    mThreads.insert(thread);
}

OsStatus sys2::Process::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (auto process = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, loanWeak(), system->pageTables(), pteMemory)) {
        ProcessHandle *result = new (std::nothrow) ProcessHandle(process, newHandleId(eOsHandleProcess), ProcessAccess::eAll);
        if (!result) {
            system->releaseMapping(pteMemory);
            return OsStatusOutOfMemory;
        }

        addHandle(result);
        addChild(process);

        system->addProcessObject(process);
        *handle = result;
        return OsStatusSuccess;
    }

    system->releaseMapping(pteMemory);
    return OsStatusOutOfMemory;
}

OsStatus sys2::Process::destroy(System *system, const ProcessDestroyInfo& info) {
    if (auto parent = mParent.lock()) {
        parent->removeChild(loanShared());
    }

    mState = info.reason;
    mExitCode = info.exitCode;

    for (sm::RcuSharedPtr<Process> child : mChildren) {
        if (OsStatus status = child->destroy(system, ProcessDestroyInfo { .exitCode = info.exitCode, .reason = eOsProcessOrphaned })) {
            return status;
        }
    }

    for (sm::RcuSharedPtr<Thread> thread : mThreads) {
        if (OsStatus status = thread->destroy(system, eOsThreadOrphaned)) {
            return status;
        }
    }

    mChildren.clear();

    for (km::MemoryRange range : mPhysicalMemory) {
        system->releaseMemory(range);
    }

    mPhysicalMemory.clear();

    mHandles.clear();

    if (OsStatus status = system->releaseMapping(mPteMemory)) {
        return status;
    }

    system->removeProcessObject(loanWeak());

    return OsStatusSuccess;
}

OsStatus sys2::Process::vmemCreate(System *system, OsVmemCreateInfo info, km::AddressMapping *mapping) {
    // The size must be a multiple of the smallest page size and must not be 0.
    if ((info.Size == 0) || (info.Size % x64::kPageSize != 0)) {
        return OsStatusInvalidInput;
    }

    // Mappings must be page aligned, unless it is 0,
    if ((info.Alignment != 0) && (info.Alignment % x64::kPageSize != 0)) {
        return OsStatusInvalidInput;
    }

    uintptr_t base = (uintptr_t)info.BaseAddress;
    OsSize alignment = info.Alignment == 0 ? x64::kPageSize : info.Alignment;
    bool isHint = (info.Access & eOsMemoryAddressHint) != 0;

    //
    // If we should interpret the base address as a hint then it must not be 0 and must be aligned
    // to the specified alignment.
    //
    if (isHint) {
        if (base == 0 || (base % alignment != 0)) {
            return OsStatusInvalidInput;
        }
    } else {
        //
        // If the base address is not a hint then it must either be 0, or must be aligned to the
        // specified alignment.
        //
        if ((base != 0) && (base % alignment != 0)) {
            return OsStatusInvalidInput;
        }
    }

    km::PageFlags flags = km::PageFlags::eUser;
    if (info.Access & eOsMemoryRead) {
        flags |= km::PageFlags::eRead;
    }

    if (info.Access & eOsMemoryWrite) {
        flags |= km::PageFlags::eWrite;
    }

    if (info.Access & eOsMemoryExecute) {
        flags |= km::PageFlags::eExecute;
    }

    km::MemoryRange range = system->mPageAllocator->alloc4k(km::Pages(info.Size));
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mPageTables.map(range, flags, km::MemoryType::eWriteBack, mapping)) {
        system->mPageAllocator->release(range);
        return status;
    }

    mPhysicalMemory.add(range);

    return OsStatusSuccess;
}

OsStatus sys2::Process::vmemRelease(System *, km::VirtualRange) {
    return OsStatusNotSupported;
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

OsStatus sys2::SysCreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    OsHandle id = OS_HANDLE_NEW(eOsHandleProcess, 0);
    ProcessHandle *result = nullptr;

    if (OsStatus status = CreateProcessInner(system, info, nullptr, id, &result)) {
        return status;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus sys2::SysCreateProcess(InvokeContext *context, ProcessCreateInfo info, ProcessHandle **handle) {
    ProcessHandle *parent = info.process;
    TxHandle *tx = context->tx;
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

OsStatus sys2::SysProcessStat(InvokeContext *, ProcessStatInfo info, ProcessStat *result) {
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
