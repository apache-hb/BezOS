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

sys2::Process::Process(ObjectName name, OsProcessStateFlags state, sm::RcuWeakPtr<Process> parent, OsProcessId pid, const km::AddressSpace *systemTables, km::AddressMapping pteMemory)
    : Super(name)
    , mId(ProcessId(pid))
    , mState(state)
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

    *info = ProcessStat {
        .name = getNameUnlocked(),
        .id = OsProcessId(mId),
        .exitCode = mExitCode,
        .state = mState,
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

    return OsStatusNotFound;
}

OsStatus sys2::Process::currentHandle(OsProcessAccess access, OsProcessHandle *handle) {
    return resolveObject(loanShared(), access, handle);
}

OsStatus sys2::Process::resolveObject(sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle) {
    HandleCreateInfo createInfo {
        .owner = loanShared(),
        .access = access,
    };

    IHandle *result = nullptr;
    if (OsStatus status = object->open(createInfo, &result)) {
        return status;
    }

    *handle = result->getHandle();
    return OsStatusSuccess;
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

    if (auto process = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info.name, info.state, loanWeak(), system->nextProcessId(), system->pageTables(), pteMemory)) {
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

static OsStatus CreateProcessInner(sys2::System *system, sys2::ObjectName name, OsProcessStateFlags state, sm::RcuSharedPtr<sys2::Process> parent, OsHandle id, sys2::ProcessHandle **handle) {
    sm::RcuSharedPtr<sys2::Process> process;
    sys2::ProcessHandle *result = nullptr;
    km::AddressMapping pteMemory;

    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    // Create the process.
    process = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), name, state, parent, system->nextProcessId(), system->pageTables(), pteMemory);
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

    if (OsStatus status = CreateProcessInner(system, info.name, info.state, nullptr, id, &result)) {
        return status;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyRootProcess(System *system, ProcessHandle *handle) {
    if (!handle->hasAccess(ProcessAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    ProcessDestroyInfo info {
        .object = handle,
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    return handle->destroyProcess(system, info);
}

OsStatus sys2::SysResolveObject(InvokeContext *context, sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle) {
    return context->process->resolveObject(object, access, handle);
}

OsStatus sys2::SysCreateProcess(InvokeContext *context, OsProcessCreateInfo info, OsProcessHandle *handle) {
    ProcessHandle *hParent = nullptr;
    sm::RcuSharedPtr<Process> process;

    OsProcessHandle hParentHandle = info.Parent;
    if (hParentHandle != OS_HANDLE_INVALID) {
        if (OsStatus status = SysFindHandle(context, hParentHandle, &hParent)) {
            return status;
        }

        process = hParent->getProcess();
    } else {
        process = context->process;
    }

    return SysCreateProcess(context, process, info, handle);
}

OsStatus sys2::SysCreateProcess(InvokeContext *context, sm::RcuSharedPtr<Process> parent, OsProcessCreateInfo info, OsProcessHandle *handle) {
    ProcessHandle *hResult = nullptr;
    TxHandle *hTx = nullptr;

    // If there is a transaction, ensure we can append to it.
    if ((context->tx != OS_HANDLE_INVALID)) {
        if (OsStatus status = SysFindHandle(context, context->tx, &hTx)) {
            return status;
        }
    }

    OsHandle id = context->process->newHandleId(eOsHandleProcess);
    if (OsStatus status = CreateProcessInner(context->system, info.Name, info.Flags, parent, id, &hResult)) {
        return status;
    }

    parent->addChild(hResult->getProcess());
    context->process->addHandle(hResult);
    *handle = hResult->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysDestroyProcess(InvokeContext *context, OsProcessHandle handle, int64_t exitCode, OsProcessStateFlags reason) {
    sys2::ProcessHandle *hProcess = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hProcess)) {
        return status;
    }

    return SysDestroyProcess(context, hProcess, exitCode, reason);
}

OsStatus sys2::SysDestroyProcess(InvokeContext *context, sys2::ProcessHandle *handle, int64_t exitCode, OsProcessStateFlags reason) {
    if (!handle->hasAccess(ProcessAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    ProcessDestroyInfo info {
        .object = handle,
        .exitCode = exitCode,
        .reason = reason,
    };

    return handle->destroyProcess(context->system, info);
}

OsStatus sys2::SysProcessStat(InvokeContext *context, OsProcessHandle handle, OsProcessInfo *result) {
    sys2::ProcessHandle *hProcess = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hProcess)) {
        return status;
    }

    if (!hProcess->hasAccess(ProcessAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> proc = hProcess->getProcess();
    ProcessStat info;
    if (OsStatus status = proc->stat(&info)) {
        return status;
    }

    OsProcessInfo processInfo {
        .Parent = info.parent,
        .Id = info.id,
        .Status = info.state,
        .ExitCode = info.exitCode,
    };
    size_t size = std::min(sizeof(processInfo.Name), info.name.count());
    std::memcpy(processInfo.Name, info.name.data(), size);

    *result = processInfo;

    return OsStatusSuccess;
}

OsStatus sys2::SysProcessCurrent(InvokeContext *context, OsProcessAccess access, OsProcessHandle *handle) {
    return context->process->currentHandle(access, handle);
}

OsStatus sys2::SysQueryProcessList(InvokeContext *context, ProcessQueryInfo info, ProcessQueryResult *result) {
    sys2::System *system = context->system;
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
            .owner = context->process,
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

bool sys2::IsParentProcess(sm::RcuSharedPtr<Process> process, sm::RcuSharedPtr<Process> parent) {
    sm::RcuSharedPtr<Process> current = process;
    while (current) {
        if (current == parent) {
            return true;
        }

        current = current->lockParent();
    }

    return false;
}

OsStatus sys2::SysGetInvokerTx(InvokeContext *context, TxHandle **outHandle) {
    if (context->tx == OS_HANDLE_INVALID) {
        return OsStatusInvalidHandle;
    }

    return SysFindHandle(context, context->tx, outHandle);
}
