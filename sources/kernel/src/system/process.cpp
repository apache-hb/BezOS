#include "system/process.hpp"
#include "arch/paging.hpp"
#include "fs/utils.hpp"
#include "memory.hpp"
#include "memory/layout.hpp"
#include "memory/range.hpp"
#include "system/system.hpp"
#include "system/device.hpp"
#include "fs/interface.hpp"
#include "system/sanitize.hpp"
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
    AddressSpaceManager::setActiveMap(&mAddressSpace);
}

void sys::SetProcessInitArgs(sys::System *system, sys::Process *process, std::span<std::byte> args) {
    if (args.empty()) {
        process->mInitArgsMapping = km::AddressMapping {};
        process->mInitArgsSize = 0;
    } else {
        km::AddressMapping data;

        OsStatus status = process->vmemCreate(system, VmemCreateInfo {
            .size = sm::roundup(args.size(), x64::kPageSize),
            .alignment = x64::kPageSize,
            .flags = km::PageFlags::eUser | km::PageFlags::eRead,
            .zeroMemory = true,
            .mode = sys::VmemCreateMode::eCommit,
        }, &data);
        KM_ASSERT(status == OsStatusSuccess);

        km::TlsfAllocation allocation;
        status = system->mSystemTables->map(data.physicalRangeEx(), km::PageFlags::eData, km::MemoryType::eWriteCombine, &allocation);
        KM_ASSERT(status == OsStatusSuccess);

        auto range = allocation.range().cast<sm::VirtualAddress>();
        memcpy((void*)range.front.address, args.data(), args.size());

        status = system->mSystemTables->unmap(allocation);
        KM_ASSERT(status == OsStatusSuccess);

        process->mInitArgsMapping = data;
        process->mInitArgsSize = args.size();
    }
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

        SetProcessInitArgs(system, process.get(), info.initArgs);

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
    KmDebugMessage("[SYS] Destroying process: ", getName(), " ", (info.reason), " - ", info.exitCode, "\n");

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

OsStatus sys::Process::vmemCreate(System *system, VmemCreateInfo info, km::AddressMapping *mapping) {
    km::MemoryRange memory;

    if (OsStatus status = system->mMemoryManager.allocate(info.size, info.alignment, &memory)) {
        return status;
    }

    km::AddressMapping result;
    if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, info.size, info.alignment, info.flags, km::MemoryType::eWriteBack, &result)) {
        OsStatus inner = system->mMemoryManager.release(memory);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    if (info.zeroMemory) {
        km::AddressMapping kernelMapping;
        if (OsStatus status = system->mSystemTables->map(memory, km::PageFlags::eData, km::MemoryType::eWriteBack, &kernelMapping)) {
            KM_ASSERT(status == OsStatusSuccess);
        }

        memset((void*)kernelMapping.vaddr, 0, kernelMapping.size);

        if (OsStatus status = system->mSystemTables->unmap(kernelMapping.virtualRange())) {
            KM_ASSERT(status == OsStatusSuccess);
        }
    }

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus sys::Process::vmemMapFile(System *system, VmemMapInfo info, vfs::IFileHandle *fileHandle, km::VirtualRange *result) {
    km::MemoryRange memory;

    if (OsStatus status = MapFileToMemory(fileHandle, &system->mMemoryManager, system->mSystemTables, info.srcAddress.address, info.size, &memory)) {
        return status;
    }

    km::AddressMapping mapping;

    if (info.baseAddress.isNull()) {
        if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, memory, info.flags, km::MemoryType::eWriteBack, &mapping)) {
            KmDebugMessage("[VMEM] Failed to allocate file mapping: ", info.baseAddress, " ", memory, " ", OsStatusId(status), "\n");
            OsStatus inner = system->mMemoryManager.release(memory);
            KM_ASSERT(inner == OsStatusSuccess);
            return status;
        }
    } else {
        if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, info.baseAddress, memory, info.flags, km::MemoryType::eWriteBack, &mapping)) {
            KmDebugMessage("[VMEM] Failed to map file: ", info.baseAddress, " ", memory, " ", OsStatusId(status), "\n");
            OsStatus inner = system->mMemoryManager.release(memory);
            KM_ASSERT(inner == OsStatusSuccess);
            return status;
        }
    }

    *result = mapping.virtualRange();
    return OsStatusSuccess;
}

OsStatus sys::Process::vmemMapProcess(System *system, VmemMapInfo info, sm::RcuSharedPtr<Process> process, km::VirtualRange *mapping) {
    km::VirtualRange vm = km::VirtualRange::of((void*)info.srcAddress, info.size);

    if (OsStatus status = mAddressSpace.map(&system->mMemoryManager, &process->mAddressSpace, vm, info.flags, km::MemoryType::eWriteBack, mapping)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::Process::vmemRelease(System *, km::VirtualRange) {
    return OsStatusNotSupported;
}

static OsStatus CreateProcessInner(sys::System *system, sys::ObjectName name, OsProcessStateFlags state, sm::RcuSharedPtr<sys::Process> parent, std::span<std::byte> args, OsHandle id, sys::ProcessHandle **handle) {
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

    SetProcessInitArgs(system, process.get(), args);

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

    if (OsStatus status = CreateProcessInner(system, info.name, info.state, nullptr, std::span<std::byte>{}, id, &result)) {
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

    std::span<std::byte> args {
        (std::byte*)info.ArgsBegin,
        (std::byte*)info.ArgsEnd,
    };

    OsHandle id = context->process->newHandleId(eOsHandleProcess);
    if (OsStatus status = CreateProcessInner(context->system, info.Name, info.Flags, parent, args, id, &hResult)) {
        return status;
    }

    parent->addChild(hResult->getProcess());
    context->process->addHandle(hResult);
    *handle = hResult->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysProcessDestroy(InvokeContext *context, OsProcessHandle handle, int64_t exitCode, OsProcessStateFlags reason) {
    sys::ProcessHandle *hProcess = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &hProcess)) {
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
    sm::RcuSharedPtr<Process> proc;

    if (handle == OS_HANDLE_INVALID) {
        proc = context->process;
    } else {
        sys::ProcessHandle *hProcess = nullptr;
        if (OsStatus status = SysFindHandle(context, handle, &hProcess)) {
            return status;
        }

        if (!hProcess->hasAccess(ProcessAccess::eStat)) {
            return OsStatusAccessDenied;
        }

        proc = hProcess->getProcess();
    }

    ProcessStat info;
    if (OsStatus status = proc->stat(&info)) {
        return status;
    }

    OsProcessInfo processInfo {
        .Parent = info.parent,
        .Id = info.id,
        .ArgsBegin = std::bit_cast<const OsProcessParam*>(info.args.data()),
        .ArgsEnd = std::bit_cast<const OsProcessParam*>(info.args.data() + info.args.size()),
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
    sys::VmemCreateInfo vmemInfo;
    if (OsStatus status = sys::sanitize(context, &info, &vmemInfo)) {
        return status;
    }

    auto process = vmemInfo.process;

    km::AddressMapping mapping;
    if (OsStatus status = process->vmemCreate(context->system, vmemInfo, &mapping)) {
        return status;
    }

    KmDebugMessage("[VMEM] Created vmem ", mapping, " (", vmemInfo.flags, ") in ", process->getName(), "\n");

    *outVmem = (void*)mapping.vaddr;

    return OsStatusSuccess;
}

OsStatus sys::SysVmemRelease(InvokeContext *, OsAnyPointer, OsSize) {
    return OsStatusNotSupported;
}

OsStatus sys::SysVmemMap(InvokeContext *context, OsVmemMapInfo info, void **outVmem) {
    VmemMapInfo vmemInfo;
    if (OsStatus status = sys::sanitize(context, &info, &vmemInfo)) {
        return status;
    }

    auto process = vmemInfo.process;
    auto srcObject = vmemInfo.srcObject;
    switch (srcObject->getObjectType()) {
    case eOsHandleProcess: {
        sm::RcuSharedPtr src = sm::rcuSharedPtrCast<Process>(srcObject);
        km::VirtualRange vm;
        if (OsStatus status = process->vmemMapProcess(context->system, vmemInfo, src, &vm)) {
            return status;
        }
        KmDebugMessage("[VMEM] Mapped vmem ", vm, " from process '", src->getName(), "' into '", process->getName(), "'\n");
        *outVmem = (void*)vm.front;
        return OsStatusSuccess;
    }
    case eOsHandleDevice: {
        sm::RcuSharedPtr device = sm::rcuSharedPtrCast<Device>(srcObject);
        vfs::IFileHandle *file = static_cast<vfs::IFileHandle*>(device->getVfsHandle());
        km::VirtualRange vm;
        if (OsStatus status = process->vmemMapFile(context->system, vmemInfo, file, &vm)) {
            return status;
        }
        KmDebugMessage("[VMEM] Mapped vmem ", vm, " from file into '", process->getName(), "'\n");
        *outVmem = (void*)vm.front;
        return OsStatusSuccess;
    }
    default:
        KM_PANIC("Invalid object type");
    }
}

OsStatus sys::MapFileToMemory(vfs::IFileHandle *file, MemoryManager *mm, km::AddressSpace *pt, size_t offset, size_t size, km::MemoryRange *range) {
    km::MemoryRange result;
    if (OsStatus status = mm->allocate(sm::roundup(size, x64::kPageSize), x64::kPageSize, &result)) {
        return status;
    }

    km::AddressMapping privateMapping;
    if (OsStatus status = pt->map(result, km::PageFlags::eData, km::MemoryType::eWriteBack, &privateMapping)) {
        OsStatus inner = mm->release(result);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    defer { KM_ASSERT(pt->unmap(privateMapping.virtualRange()) == OsStatusSuccess); };

    vfs::ReadRequest request {
        .begin = sm::VirtualAddress(privateMapping.vaddr),
        .end = sm::VirtualAddress(privateMapping.vaddr) + size,
        .offset = offset,
    };
    vfs::ReadResult read{};

    if (OsStatus status = file->read(request, &read)) {
        KM_ASSERT(mm->release(result) == OsStatusSuccess);
        return status;
    }

    if (read.read != size) {
        KmDebugMessage("[VMEM] Read ", read.read, " bytes from file, expected ", size, "\n");
        KM_ASSERT(read.read == size);
    }

    *range = result;
    return OsStatusSuccess;
}
