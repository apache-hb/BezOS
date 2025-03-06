#include "process/process.hpp"
#include "kernel.hpp"
#include "log.hpp"

using namespace km;

sm::RcuSharedPtr<Thread> SystemObjects::createThread(stdx::String name, sm::RcuSharedPtr<Process> process) {
    ThreadId id = mThreadIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<Thread>(&mDomain, id, std::move(name), process);

    SystemMemory *memory = GetSystemMemory();

    AddressMapping stackMapping = memory->allocateStack(0x4000 * 4);
    ptr->syscallStack = stackMapping;

    process->addThread(ptr);

    stdx::UniqueLock guard(mLock);
    mThreads.insert({id, ptr});

    return ptr;
}

sm::RcuSharedPtr<AddressSpace> SystemObjects::createAddressSpace(stdx::String name, AddressMapping mapping, PageFlags flags, MemoryType type, sm::RcuSharedPtr<Process> process) {
    AddressSpaceId id = mAddressSpaceIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<AddressSpace>(&mDomain, id, std::move(name), mapping, flags, type);

    process->addAddressSpace(ptr);

    stdx::UniqueLock guard(mLock);
    mAddressSpaces.insert({id, ptr});
    return ptr;
}

sm::RcuSharedPtr<Process> SystemObjects::createProcess(stdx::String name, x64::Privilege privilege, SystemPageTables *systemTables, MemoryRange pteMemory) {
    AddressMapping mapping{};
    OsStatus status = systemTables->map(pteMemory, PageFlags::eData, MemoryType::eWriteBack, &mapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[PROC] Failed to map process page tables: ", status, "\n");
        return nullptr;
    }

    ProcessId id = mProcessIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<Process>(&mDomain, id, std::move(name), privilege, systemTables, mapping, DefaultUserArea());

    stdx::UniqueLock guard(mLock);

    mProcesses.insert({id, ptr});
    return ptr;
}

sm::RcuSharedPtr<Mutex> SystemObjects::createMutex(stdx::String name) {
    MutexId id = mMutexIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<Mutex>(&mDomain, id, std::move(name));

    stdx::UniqueLock guard(mLock);
    mMutexes.insert({id, ptr});
    return ptr;
}

sm::RcuWeakPtr<Thread> SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second;
    }

    return nullptr;
}

sm::RcuSharedPtr<AddressSpace> SystemObjects::getAddressSpace(AddressSpaceId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mAddressSpaces.find(id); it != mAddressSpaces.end()) {
        return it->second;
    }

    return nullptr;
}

sm::RcuSharedPtr<Process> SystemObjects::getProcess(ProcessId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mProcesses.find(id); it != mProcesses.end()) {
        return it->second;
    }

    return nullptr;
}

sm::RcuSharedPtr<Mutex> SystemObjects::getMutex(MutexId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mMutexes.find(id); it != mMutexes.end()) {
        return it->second;
    }

    return nullptr;
}
