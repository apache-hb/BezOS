#include "process/process.hpp"
#include "kernel.hpp"
#include "log.hpp"

using namespace km;

Thread * SystemObjects::createThread(stdx::String name, Process * process) {
    ThreadId id = mThreadIds.allocate();
    Thread *ptr = new Thread(id, std::move(name), process);

    SystemMemory *memory = GetSystemMemory();

    AddressMapping stackMapping = memory->allocateStack(0x4000 * 4);
    ptr->syscallStack = stackMapping;

    process->addThread(ptr);

    stdx::UniqueLock guard(mLock);
    mThreads.insert({id, ptr});

    return ptr;
}

AddressSpace * SystemObjects::createAddressSpace(stdx::String name, AddressMapping mapping, PageFlags flags, MemoryType type, Process * process) {
    AddressSpaceId id = mAddressSpaceIds.allocate();
    AddressSpace *ptr = new AddressSpace(id, std::move(name), mapping, flags, type);

    process->addAddressSpace(ptr);

    stdx::UniqueLock guard(mLock);
    mAddressSpaces.insert({id, ptr});
    return ptr;
}

Process * SystemObjects::createProcess(stdx::String name, x64::Privilege privilege, SystemPageTables *systemTables, MemoryRange pteMemory) {
    AddressMapping mapping{};
    OsStatus status = systemTables->map(pteMemory, PageFlags::eData, MemoryType::eWriteBack, &mapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[PROC] Failed to map process page tables: ", status, "\n");
        return nullptr;
    }

    ProcessId id = mProcessIds.allocate();
    Process *ptr = new Process(id, std::move(name), privilege, systemTables, mapping, DefaultUserArea());

    stdx::UniqueLock guard(mLock);

    mProcesses.insert({id, ptr});
    return ptr;
}

Mutex * SystemObjects::createMutex(stdx::String name) {
    MutexId id = mMutexIds.allocate();
    Mutex *ptr = new Mutex(id, std::move(name));

    stdx::UniqueLock guard(mLock);
    mMutexes.insert({id, ptr});
    return ptr;
}

Thread * SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second;
    }

    return nullptr;
}

AddressSpace * SystemObjects::getAddressSpace(AddressSpaceId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mAddressSpaces.find(id); it != mAddressSpaces.end()) {
        return it->second;
    }

    return nullptr;
}

Process * SystemObjects::getProcess(ProcessId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mProcesses.find(id); it != mProcesses.end()) {
        return it->second;
    }

    return nullptr;
}

Mutex * SystemObjects::getMutex(MutexId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mMutexes.find(id); it != mMutexes.end()) {
        return it->second;
    }

    return nullptr;
}
