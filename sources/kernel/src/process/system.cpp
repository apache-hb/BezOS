#include "process/system.hpp"

#include "kernel.hpp"
#include "log.hpp"

using namespace km;

Thread *SystemObjects::createThread(stdx::String name, Process *process) {
    ThreadId id = mThreadIds.allocate();
    Thread *ptr = new Thread(id, std::move(name), process);

    AddressMapping stackMapping = mMemory->allocateStack(0x4000 * 4);
    ptr->syscallStack = stackMapping;

    process->addThread(ptr);

    stdx::UniqueLock guard(mLock);
    mThreads.insert({id, ptr});

    return ptr;
}

AddressSpace *SystemObjects::createAddressSpace(stdx::String name, AddressMapping mapping, PageFlags flags, MemoryType type, Process *process) {
    AddressSpaceId id = mAddressSpaceIds.allocate();
    AddressSpace *ptr = new AddressSpace(id, std::move(name), mapping, flags, type);

    process->addAddressSpace(ptr);

    stdx::UniqueLock guard(mLock);
    mAddressSpaces.insert({id, ptr});
    return ptr;
}

OsStatus SystemObjects::createProcess(stdx::String name, x64::Privilege privilege, MemoryRange pteMemory, Process **process) {
    SystemPageTables& systemTables = mMemory->pageTables();

    std::unique_ptr<Process> result{ new (std::nothrow) Process };
    if (result == nullptr) {
        return OsStatusOutOfMemory;
    }

    AddressMapping pteMapping{};
    if (OsStatus status = systemTables.map(pteMemory, PageFlags::eData, MemoryType::eWriteBack, &pteMapping)) {
        KmDebugMessage("[PROC] Failed to map process page tables: ", status, "\n");
        return status;
    }

    ProcessId id = mProcessIds.allocate();
    result->init(id, std::move(name), privilege, &systemTables, pteMapping, DefaultUserArea());

    Process *handle = result.release();

    stdx::UniqueLock guard(mLock);

    mProcesses.insert({id, handle});

    *process = handle;
    return OsStatusSuccess;
}

OsStatus SystemObjects::destroyProcess(Process *process) {
    stdx::UniqueLock guard(mLock);

    if (auto it = mProcesses.find(ProcessId(process->internalId())); it == mProcesses.end()) {
        return OsStatusNotFound;
    } else {
        if (releaseHandle(process)) {
            mProcesses.erase(it);
        }
    }

    return OsStatusSuccess;
}

Process *SystemObjects::getProcess(ProcessId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mProcesses.find(id); it != mProcesses.end()) {
        return it->second;
    }

    return nullptr;
}

OsStatus SystemObjects::exitProcess(Process *process, int64_t status) {
    OsProcessStateFlags state = (process->state.Status & ~eOsProcessStatusMask) | eOsProcessExited;
    process->state.Status = state;
    process->state.ExitCode = status;
    return OsStatusSuccess;
}

Mutex *SystemObjects::createMutex(stdx::String name) {
    MutexId id = mMutexIds.allocate();
    Mutex *ptr = new Mutex(id, std::move(name));

    stdx::UniqueLock guard(mLock);
    mMutexes.insert({id, ptr});
    return ptr;
}

Thread *SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second;
    }

    return nullptr;
}

AddressSpace *SystemObjects::getAddressSpace(AddressSpaceId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mAddressSpaces.find(id); it != mAddressSpaces.end()) {
        return it->second;
    }

    return nullptr;
}

Mutex *SystemObjects::getMutex(MutexId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mMutexes.find(id); it != mMutexes.end()) {
        return it->second;
    }

    return nullptr;
}

bool SystemObjects::releaseHandle(KernelObject *handle) {
    if (handle->releaseStrong()) {
        delete handle;
        return true;
    }

    return false;
}
