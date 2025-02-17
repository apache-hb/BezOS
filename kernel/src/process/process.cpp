#include "process/process.hpp"

using namespace km;

Thread *SystemObjects::createThread(stdx::String name, Process *process) {
    stdx::UniqueLock guard(mLock);

    ThreadId id = mThreadIds.allocate();
    std::unique_ptr<Thread> thread{new Thread{id, std::move(name), process}};
    Thread *result = thread.get();
    mThreads.insert({id, std::move(thread)});
    return result;
}

AddressSpace *SystemObjects::createAddressSpace(stdx::String name, km::AddressMapping mapping) {
    stdx::UniqueLock guard(mLock);

    AddressSpaceId id = mAddressSpaceIds.allocate();
    std::unique_ptr<AddressSpace> addressSpace{new AddressSpace{id, std::move(name), mapping}};
    AddressSpace *result = addressSpace.get();
    mAddressSpaces.insert({id, std::move(addressSpace)});
    return result;
}

Process *SystemObjects::createProcess(stdx::String name, km::Privilege privilege) {
    stdx::UniqueLock guard(mLock);

    ProcessId id = mProcessIds.allocate();
    std::unique_ptr<Process> process{new Process{id, std::move(name), privilege}};
    Process *result = process.get();
    mProcesses.insert({id, std::move(process)});
    return result;
}

Mutex *SystemObjects::createMutex(stdx::String name) {
    stdx::UniqueLock guard(mLock);

    MutexId id = mMutexIds.allocate();
    std::unique_ptr<Mutex> mutex{new Mutex{id, std::move(name)}};
    Mutex *result = mutex.get();
    mMutexes.insert({id, std::move(mutex)});
    return result;
}

Thread *SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second.get();
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

Process *SystemObjects::getProcess(ProcessId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mProcesses.find(id); it != mProcesses.end()) {
        return it->second.get();
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
