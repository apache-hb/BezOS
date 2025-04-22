#include "system/process.hpp"
#include "arch/paging.hpp"
#include "memory.hpp"
#include "memory/layout.hpp"
#include "memory/range.hpp"
#include "system/system.hpp"
#include "system/device.hpp"
#include "fs2/interface.hpp"
#include "system/vm/file.hpp"
#include "system/vm/memory.hpp"

#include <bezos/handle.h>

OsStatus sys2::ProcessHandle::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    if (!hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    return getInner()->createProcess(system, info, handle);
}

OsStatus sys2::ProcessHandle::destroyProcess(System *system, const ProcessDestroyInfo& info) {
    if (!hasAccess(ProcessAccess::eDestroy)) {
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

    return OsStatusInvalidHandle;
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

OsStatus sys2::Process::resolveAddress(const void *address, sm::RcuSharedPtr<IMemoryObject> *object) {
    if ((uintptr_t)address % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    if (auto it = mMemoryObjects.find(address); it != mMemoryObjects.end()) {
        *object = it->second;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
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

void sys2::Process::loadPageTables() {
    auto pm = mPageTables.pageManager();
    pm->setActiveMap(mPageTables.root());
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

        km::VirtualRange vm;
        if (OsStatus status = mPageTables.reserve(info.Size, &vm)) {
            return status;
        }

        // TODO: leaks on error path, also doesnt use hint address
        base = (uintptr_t)vm.front;
    } else {
        //
        // If the base address is not a hint then it must either be 0, or must be aligned to the
        // specified alignment.
        //
        if (base == 0) {
            km::VirtualRange vm;
            if (OsStatus status = mPageTables.reserve(info.Size, &vm)) {
                return status;
            }
            // TODO: leaks on error path
            base = (uintptr_t)vm.front;
        } else {
            if (base % alignment != 0) {
                return OsStatusInvalidInput;
            }

            for (auto i = base; i < base + info.Size; i += x64::kPageSize) {
                if (mMemoryObjects.contains((void*)i)) {
                    return OsStatusInvalidInput;
                }
            }
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

    km::AddressMapping result = {
        .vaddr = (void*)base,
        .paddr = range.front,
        .size = info.Size,
    };
    if (OsStatus status = mPageTables.reserve(result, flags, km::MemoryType::eWriteBack)) {
        system->mPageAllocator->release(range);
        return status;
    }

    if (info.Access & eOsMemoryDiscard) {
        km::AddressMapping discardMapping;
        if (OsStatus status = system->mSystemTables->map(range, km::PageFlags::eData, km::MemoryType::eWriteBack, &discardMapping)) {
            system->mPageAllocator->release(range);
            return status;
        }

        memset((void*)discardMapping.vaddr, 0, discardMapping.size);
        if (OsStatus status = system->mSystemTables->unmap(discardMapping.virtualRange())) {
            system->mPageAllocator->release(range);
            return status;
        }
    }

    auto object = sm::rcuMakeShared<sys2::MemoryObject>(&system->rcuDomain(), range);

    auto vm = result.virtualRange();
    for (auto i = vm.front; i < vm.back; i = (char*)i + x64::kPageSize) {
        mMemoryObjects.insert({ (void*)i, object });
    }

    *mapping = result;
    mPhysicalMemory.add(range);

    return OsStatusSuccess;
}

OsStatus sys2::Process::vmemMapFile(System *system, OsVmemMapInfo info, vfs2::IFileHandle *fileHandle, km::VirtualRange *result) {
    sm::RcuSharedPtr<sys2::FileMapping> fileMapping;
    if (OsStatus status = sys2::MapFileToMemory(&system->rcuDomain(), fileHandle, system->mPageAllocator, system->mSystemTables, info.SrcAddress, info.SrcAddress + info.Size, &fileMapping)) {
        return status;
    }

    uintptr_t size = sm::roundup(info.Size, x64::kPageSize);
    km::VirtualRange vm;

    if (info.DstAddress == 0) {
        if (OsStatus status = mPageTables.reserve(size, &vm)) {
            return status;
        }
    } else {
        if (info.DstAddress % x64::kPageSize != 0) {
            return OsStatusInvalidInput;
        }

        vm = km::VirtualRange::of((void*)info.DstAddress, size);
        mPageTables.reserve(vm);

#if 0
        for (auto i = base; i < base + size; i += x64::kPageSize) {
            if (mMemoryObjects.contains((void*)i)) {
                return OsStatusInvalidInput;
            }
        }
#endif
    }

    km::MemoryRange range = fileMapping->range();
    km::AddressMapping mapping {
        .vaddr = vm.front,
        .paddr = range.front,
        .size = size,
    };
    if (OsStatus status = mPageTables.reserve(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack)) {
        system->mPageAllocator->release(range);
        return status;
    }

    for (auto i = vm.front; i < vm.back; i = (char*)i + x64::kPageSize) {
        mMemoryObjects.insert({ i, fileMapping });
    }

    *result = mapping.virtualRange();
    return OsStatusSuccess;
}

OsStatus sys2::Process::vmemMapProcess(OsVmemMapInfo info, sm::RcuSharedPtr<Process> process, km::VirtualRange *mapping) {
    if (info.Size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    // TODO: can leak on error path
    if (info.DstAddress == 0) {
        km::VirtualRange range;
        if (OsStatus status = mPageTables.reserve(info.Size, &range)) {
            return status;
        }

        info.DstAddress = (uintptr_t)range.front;
    }

    auto vm = km::VirtualRange::of((void*)info.DstAddress, info.Size);
    mPageTables.reserve(vm);

    for (size_t i = 0; i < info.Size / x64::kPageSize; i++) {
        auto srcAddress = (void*)(info.SrcAddress + i * x64::kPageSize);
        auto dstAddress = (void*)(info.DstAddress + i * x64::kPageSize);

        sm::RcuSharedPtr<IMemoryObject> object;
        if (OsStatus status = process->resolveAddress(srcAddress, &object)) {
            return status;
        }

        auto backing = process->mPageTables.getBackingAddress(srcAddress);
        km::AddressMapping mapping {
            .vaddr = (void*)dstAddress,
            .paddr = backing,
            .size = x64::kPageSize,
        };

        if (OsStatus status = mPageTables.reserve(mapping, km::PageFlags::eUserAll, km::MemoryType::eWriteBack)) {
            return status;
        }
    }

    *mapping = vm;
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
    if (!handle->hasAccess(ProcessAccess::eDestroy)) {
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

OsStatus sys2::SysProcessCreate(InvokeContext *context, OsProcessCreateInfo info, OsProcessHandle *handle) {
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

    return SysProcessCreate(context, process, info, handle);
}

OsStatus sys2::SysProcessCreate(InvokeContext *context, sm::RcuSharedPtr<Process> parent, OsProcessCreateInfo info, OsProcessHandle *handle) {
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

OsStatus sys2::SysProcessDestroy(InvokeContext *context, OsProcessHandle handle, int64_t exitCode, OsProcessStateFlags reason) {
    sys2::ProcessHandle *hProcess = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hProcess)) {
        return status;
    }

    return SysProcessDestroy(context, hProcess, exitCode, reason);
}

OsStatus sys2::SysProcessDestroy(InvokeContext *context, sys2::ProcessHandle *handle, int64_t exitCode, OsProcessStateFlags reason) {
    if (!handle->hasAccess(ProcessAccess::eDestroy)) {
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

OsStatus sys2::SysVmemCreate(InvokeContext *context, OsVmemCreateInfo info, void **outVmem) {
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

    km::AddressMapping mapping;
    if (OsStatus status = process->vmemCreate(context->system, info, &mapping)) {
        return status;
    }

    KmDebugMessage("[VMEM] Created vmem ", mapping, " in ", process->getName(), "\n");

    *outVmem = (void*)mapping.vaddr;

    return OsStatusSuccess;
}

OsStatus sys2::SysVmemRelease(InvokeContext *, OsAnyPointer, OsSize) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysVmemMap(InvokeContext *context, OsVmemMapInfo info, void **outVmem) {
    sm::RcuSharedPtr<Process> dstProcess;

    if (info.Source == OS_HANDLE_INVALID) {
        return OsStatusInvalidHandle;
    }

    if (info.Process != OS_HANDLE_INVALID) {
        ProcessHandle *hProcess = nullptr;
        if (OsStatus status = SysFindHandle(context, info.Process, &hProcess)) {
            return status;
        }

        dstProcess = hProcess->getProcess();
    } else {
        dstProcess = context->process;
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

    if (OS_HANDLE_TYPE(info.Source) == eOsHandleProcess) {
        ProcessHandle *hSource = nullptr;
        if (OsStatus status = SysFindHandle(context, info.Source, &hSource)) {
            return status;
        }

        sm::RcuSharedPtr<IMemoryObject> srcObject;

        auto srcProcess = hSource->getProcess();

        if (OsStatus status = srcProcess->resolveAddress((void*)info.SrcAddress, &srcObject)) {
            KmDebugMessage("[VMEM] Failed to resolve source address ", (void*)info.SrcAddress, " in process ", srcProcess->getName(), "\n");
            return status;
        }

        km::VirtualRange vm;
        if (OsStatus status = dstProcess->vmemMapProcess(info, srcProcess, &vm)) {
            return status;
        }

        *outVmem = (void*)vm.front;
        return OsStatusSuccess;
    } else if (OS_HANDLE_TYPE(info.Source) == eOsHandleDevice) {
        DeviceHandle *hDevice = nullptr;
        if (OsStatus status = SysFindHandle(context, info.Source, &hDevice)) {
            return status;
        }

        auto device = hDevice->getDevice();
        auto vfsHandle = device->getVfsHandle();

        vfs2::HandleInfo hInfo = vfsHandle->info();
        if (hInfo.guid != sm::uuid(kOsFileGuid)) {
            return OsStatusInvalidHandle;
        }

        km::VirtualRange vm;

        if (OsStatus status = dstProcess->vmemMapFile(context->system, info, static_cast<vfs2::IFileHandle*>(vfsHandle), &vm)) {
            return status;
        }

        *outVmem = (void*)vm.front;
        return OsStatusSuccess;
    } else {
        return OsStatusInvalidHandle;
    }
}
