#include "process/process.hpp"

using namespace km;

sm::RcuSharedPtr<Thread> SystemObjects::createThread(stdx::String name, sm::RcuSharedPtr<Process> process) {
    ThreadId id = mThreadIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<Thread>(&mDomain, id, std::move(name), process);

    stdx::UniqueLock guard(mLock);

    mThreads.insert({id, ptr});
    return ptr;
}

AddressSpace *SystemObjects::createAddressSpace(stdx::String name, km::AddressMapping mapping) {
    stdx::UniqueLock guard(mLock);

    AddressSpaceId id = mAddressSpaceIds.allocate();
    std::unique_ptr<AddressSpace> addressSpace{new AddressSpace{id, std::move(name), mapping}};
    AddressSpace *result = addressSpace.get();
    mAddressSpaces.insert({id, std::move(addressSpace)});
    return result;
}

sm::RcuSharedPtr<Process> SystemObjects::createProcess(stdx::String name, x64::Privilege privilege) {
    ProcessId id = mProcessIds.allocate();
    sm::RcuSharedPtr ptr = sm::rcuMakeShared<Process>(&mDomain, id, std::move(name), privilege);

    stdx::UniqueLock guard(mLock);

    mProcesses.insert({id, ptr});
    return ptr;
}

Mutex *SystemObjects::createMutex(stdx::String name) {
    stdx::UniqueLock guard(mLock);

    MutexId id = mMutexIds.allocate();
    std::unique_ptr<Mutex> mutex{new Mutex{id, std::move(name)}};
    Mutex *result = mutex.get();
    mMutexes.insert({id, std::move(mutex)});
    return result;
}

sm::RcuSharedPtr<Thread> SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second;
    }

    return nullptr;
}

AddressSpace *SystemObjects::getAddressSpace(AddressSpaceId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mAddressSpaces.find(id); it != mAddressSpaces.end()) {
        return it->second.get();
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

Mutex *SystemObjects::getMutex(MutexId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mMutexes.find(id); it != mMutexes.end()) {
        return it->second.get();
    }

    return nullptr;
}
