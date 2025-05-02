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
#include "memory/address_space.hpp"
#include "system/pmm.hpp"
#include "common/util/defer.hpp"

#include <bezos/handle.h>

OsStatus sys::ProcessHandle::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    if (!hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    return getInner()->createProcess(system, info, handle);
}

OsStatus sys::ProcessHandle::destroyProcess(System *system, const ProcessDestroyInfo& info) {
    if (!hasAccess(ProcessAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    return getInner()->destroy(system, info);
}

sys::Process::Process(
    ObjectName name,
    OsProcessStateFlags state,
    sm::RcuWeakPtr<Process> parent,
    OsProcessId pid,
    AddressSpaceManager&& addressSpace
)
    : Super(name)
    , mId(ProcessId(pid))
    , mState(state)
    , mExitCode(0)
    , mParent(parent)
    , mAddressSpace(std::move(addressSpace))
{ }

OsStatus sys::Process::stat(ProcessStat *info) {
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

OsStatus sys::Process::open(HandleCreateInfo createInfo, IHandle **handle) {
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

sys::IHandle *sys::Process::getHandle(OsHandle handle) {
    stdx::SharedLock guard(mLock);
    if (auto it = mHandles.find(handle); it != mHandles.end()) {
        return it->second.get();
    }

    return nullptr;
}

void sys::Process::addHandle(IHandle *handle) {
    stdx::UniqueLock guard(mLock);
    mHandles.insert({ handle->getHandle(), std::unique_ptr<IHandle>(handle) });
}

OsStatus sys::Process::removeHandle(IHandle *handle) {
    return removeHandle(handle->getHandle());
}

OsStatus sys::Process::removeHandle(OsHandle handle) {
    stdx::UniqueLock guard(mLock);
    if (auto it = mHandles.find(handle); it != mHandles.end()) {
        mHandles.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusInvalidHandle;
}

OsStatus sys::Process::findHandle(OsHandle handle, OsHandleType type, IHandle **result) {
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

OsStatus sys::Process::currentHandle(OsProcessAccess access, OsProcessHandle *handle) {
    return resolveObject(loanShared(), access, handle);
}

OsStatus sys::Process::resolveObject(sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle) {
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

OsStatus sys::Process::resolveAddress(const void *address, sm::RcuSharedPtr<IMemoryObject> *object) {
    if ((uintptr_t)address % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    if (auto it = mMemoryObjects.find(address); it != mMemoryObjects.end()) {
        *object = it->second;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

void sys::Process::removeChild(sm::RcuSharedPtr<Process> child) {
    stdx::UniqueLock guard(mLock);
    mChildren.erase(child);
}

void sys::Process::addChild(sm::RcuSharedPtr<Process> child) {
    stdx::UniqueLock guard(mLock);
    mChildren.insert(child);
}

void sys::Process::removeThread(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    mThreads.erase(thread);
}

void sys::Process::addThread(sm::RcuSharedPtr<Thread> thread) {
    stdx::UniqueLock guard(mLock);
    mThreads.insert(thread);
}

void sys::Process::loadPageTables() {
    sys::AddressSpaceManager::setActiveMap(&mAddressSpace);
}

OsStatus sys::Process::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    sys::AddressSpaceManager addressSpace;
    if (OsStatus status = sys::AddressSpaceManager::create(system->mSystemTables, pteMemory, km::PageFlags::eUserAll, km::DefaultUserArea(), &addressSpace)) {
        system->releaseMapping(pteMemory);
        return status;
    }

    if (auto process = sm::rcuMakeShared<sys::Process>(&system->rcuDomain(), info.name, info.state, loanWeak(), system->nextProcessId(), std::move(addressSpace))) {
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

OsStatus sys::Process::destroy(System *system, const ProcessDestroyInfo& info) {
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
    mAddressSpace.destroy(&system->mMemoryManager);

    mHandles.clear();

    if (OsStatus status = system->releaseMapping(mAddressSpace.getPteMemory())) {
        return status;
    }

    system->removeProcessObject(loanWeak());

    return OsStatusSuccess;
}

OsStatus sys::Process::vmemCreate(System *system, OsVmemCreateInfo info, km::AddressMapping *mapping) {
    km::MemoryRange memory;

    if (OsStatus status = system->mMemoryManager.allocate(info.Size, info.Alignment, &memory)) {
        return status;
    }

    km::AddressMapping result;
    if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, info.Size, info.Alignment, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &result)) {
        OsStatus inner = system->mMemoryManager.release(memory);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    *mapping = result;
    return OsStatusSuccess;
#if 0
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

    auto object = sm::rcuMakeShared<sys::MemoryObject>(&system->rcuDomain(), range);

    auto vm = result.virtualRange();
    for (auto i = vm.front; i < vm.back; i = (char*)i + x64::kPageSize) {
        mMemoryObjects.insert({ (void*)i, object });
    }

    *mapping = result;
    mPhysicalMemory.add(range);

    return OsStatusSuccess;
#endif
}

OsStatus sys::Process::vmemMapFile(System *system, OsVmemMapInfo info, vfs2::IFileHandle *fileHandle, km::VirtualRange *result) {
    km::MemoryRange memory;

    if (OsStatus status = MapFileToMemory(fileHandle, &system->mMemoryManager, system->mSystemTables, info.SrcAddress, info.Size, km::PageFlags::eUserAll, &memory)) {
        return status;
    }

    km::AddressMapping mapping;

    if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, memory.size(), x64::kPageSize, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, &mapping)) {
        OsStatus inner = system->mMemoryManager.release(memory);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    *result = mapping.virtualRange();
    return OsStatusSuccess;

#if 0
    sm::RcuSharedPtr<sys::FileMapping> fileMapping;
    if (OsStatus status = sys::MapFileToMemory(&system->rcuDomain(), fileHandle, system->mPageAllocator, system->mSystemTables, info.SrcAddress, info.SrcAddress + info.Size, &fileMapping)) {
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
#endif
}

OsStatus sys::Process::vmemMapProcess(System *system, OsVmemMapInfo info, sm::RcuSharedPtr<Process> process, km::VirtualRange *mapping) {
    if (info.Size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    km::VirtualRange vm = km::VirtualRange::of((void*)info.SrcAddress, info.Size);

    return mAddressSpace.map(&system->mMemoryManager, &process->mAddressSpace, vm, km::PageFlags::eUserAll, km::MemoryType::eWriteBack, mapping);
}

OsStatus sys::Process::vmemRelease(System *, km::VirtualRange) {
    return OsStatusNotSupported;
}

static OsStatus CreateProcessInner(sys::System *system, sys::ObjectName name, OsProcessStateFlags state, sm::RcuSharedPtr<sys::Process> parent, OsHandle id, sys::ProcessHandle **handle) {
    sm::RcuSharedPtr<sys::Process> process;
    sys::ProcessHandle *result = nullptr;
    km::AddressMapping pteMemory;
    sys::AddressSpaceManager mm;

    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (OsStatus status = sys::AddressSpaceManager::create(system->mSystemTables, pteMemory, km::PageFlags::eUserAll, km::DefaultUserArea(), &mm)) {
        system->releaseMapping(pteMemory);
        return status;
    }

    // Create the process.
    process = sm::rcuMakeShared<sys::Process>(&system->rcuDomain(), name, state, parent, system->nextProcessId(), std::move(mm));
    if (!process) {
        goto outOfMemory;
    }

    result = new (std::nothrow) sys::ProcessHandle(process, id, sys::ProcessAccess::eAll);
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

OsStatus sys::SysCreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    OsHandle id = OS_HANDLE_NEW(eOsHandleProcess, 0);
    ProcessHandle *result = nullptr;

    if (OsStatus status = CreateProcessInner(system, info.name, info.state, nullptr, id, &result)) {
        return status;
    }

    *handle = result;
    return OsStatusSuccess;
}

OsStatus sys::SysDestroyRootProcess(System *system, ProcessHandle *handle) {
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

OsStatus sys::SysResolveObject(InvokeContext *context, sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle) {
    return context->process->resolveObject(object, access, handle);
}

OsStatus sys::SysProcessCreate(InvokeContext *context, OsProcessCreateInfo info, OsProcessHandle *handle) {
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

OsStatus sys::SysProcessCreate(InvokeContext *context, sm::RcuSharedPtr<Process> parent, OsProcessCreateInfo info, OsProcessHandle *handle) {
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

OsStatus sys::SysProcessDestroy(InvokeContext *context, OsProcessHandle handle, int64_t exitCode, OsProcessStateFlags reason) {
    sys::ProcessHandle *hProcess = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hProcess)) {
        return status;
    }

    return SysProcessDestroy(context, hProcess, exitCode, reason);
}

OsStatus sys::SysProcessDestroy(InvokeContext *context, sys::ProcessHandle *handle, int64_t exitCode, OsProcessStateFlags reason) {
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

OsStatus sys::SysProcessStat(InvokeContext *context, OsProcessHandle handle, OsProcessInfo *result) {
    sys::ProcessHandle *hProcess = nullptr;
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

OsStatus sys::SysProcessCurrent(InvokeContext *context, OsProcessAccess access, OsProcessHandle *handle) {
    return context->process->currentHandle(access, handle);
}

OsStatus sys::SysQueryProcessList(InvokeContext *context, ProcessQueryInfo info, ProcessQueryResult *result) {
    sys::System *system = context->system;
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

bool sys::IsParentProcess(sm::RcuSharedPtr<Process> process, sm::RcuSharedPtr<Process> parent) {
    sm::RcuSharedPtr<Process> current = process;
    while (current) {
        if (current == parent) {
            return true;
        }

        current = current->lockParent();
    }

    return false;
}

OsStatus sys::SysGetInvokerTx(InvokeContext *context, TxHandle **outHandle) {
    if (context->tx == OS_HANDLE_INVALID) {
        return OsStatusInvalidHandle;
    }

    return SysFindHandle(context, context->tx, outHandle);
}

OsStatus sys::SysVmemCreate(InvokeContext *context, OsVmemCreateInfo info, void **outVmem) {
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

OsStatus sys::SysVmemRelease(InvokeContext *, OsAnyPointer, OsSize) {
    return OsStatusNotSupported;
}

OsStatus sys::SysVmemMap(InvokeContext *context, OsVmemMapInfo info, void **outVmem) {
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
        if (OsStatus status = dstProcess->vmemMapProcess(context->system, info, srcProcess, &vm)) {
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

OsStatus sys::MapFileToMemory(vfs2::IFileHandle *file, MemoryManager *mm, km::AddressSpace *pt, size_t offset, size_t size, km::PageFlags flags, km::MemoryRange *range) {
    km::MemoryRange result;
    if (OsStatus status = mm->allocate(sm::roundup(size, x64::kPageSize), x64::kPageSize, &result)) {
        return status;
    }

    km::AddressMapping mapping;
    if (OsStatus status = pt->map(result, flags, km::MemoryType::eWriteBack, &mapping)) {
        OsStatus inner = mm->release(result);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    defer { KM_ASSERT(pt->unmap(mapping.virtualRange()) == OsStatusSuccess); };

    vfs2::ReadRequest request {
        .begin = sm::VirtualAddress(mapping.vaddr),
        .end = sm::VirtualAddress(mapping.vaddr) + size,
        .offset = offset,
    };
    vfs2::ReadResult read{};

    if (OsStatus status = file->read(request, &read)) {
        KM_ASSERT(mm->release(result) == OsStatusSuccess);
        return status;
    }

    *range = result;
    return OsStatusSuccess;
}
