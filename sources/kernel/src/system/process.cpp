#include "system/process.hpp"
#include "memory.hpp"
#include "system/system.hpp"

void sys2::Process::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
    mName = name;
}

sys2::ObjectName sys2::Process::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

OsStatus sys2::Process::stat(ProcessInfo *info) {
    stdx::SharedLock guard(mLock);
    *info = ProcessInfo {
        .name = mName,
        .handles = mHandles.size(),
        .supervisor = mSupervisor,
    };

    return OsStatusSuccess;
}

sys2::Process::Process(const ProcessCreateInfo& createInfo, const km::AddressSpace *systemTables, km::AddressMapping pteMemory)
    : mName(createInfo.name)
    , mSupervisor(createInfo.supervisor)
    , mPteMemory(pteMemory)
    , mPageTables(systemTables, mPteMemory, km::PageFlags::eUserAll, km::DefaultUserArea())
{ }

void sys2::Process::addChild(sm::RcuWeakPtr<Process> child) {
    sm::RcuSharedPtr<Process> handle = child.lock();
    if (!handle) {
        return;
    }

    stdx::UniqueLock guard(mLock);
    mChildren.insert(child);
    handle->mParent = loanWeak();
}

void sys2::Process::removeChild(sm::RcuWeakPtr<Process> child) {
    sm::RcuSharedPtr<Process> handle = child.lock();
    if (!handle) {
        return;
    }

    stdx::UniqueLock guard(mLock);
    auto count = mChildren.erase(child);
    KM_CHECK(count == 1, "Failed to remove child process");
    handle->mParent.reset();
}

OsStatus sys2::CreateProcess(System *system, ProcessCreateInfo info, sm::RcuSharedPtr<Process> *process) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (auto result = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, system->pageTables(), pteMemory)) {
        system->addObject(result);

        if (auto parent = info.parent.lock()) {
            parent->addChild(result);
        }

        *process = result;
        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}

OsStatus sys2::DestroyProcess(System *system, const ProcessDestroyInfo& info, sm::RcuWeakPtr<Process> process) {
    // TODO: destroy threads, close handles, destroy children, etc.
    // TODO: wait for all threads to exit

    sm::RcuSharedPtr<Process> handle = process.lock();
    if (!handle) {
        return OsStatusInvalidHandle;
    }

    for (sm::RcuWeakPtr<Process> child : handle->mChildren) {
        if (OsStatus status = DestroyProcess(system, info, child)) {
            return status;
        }
    }

    if (auto parent = handle->mParent.lock()) {
        parent->removeChild(process);
    }

    if (OsStatus status = system->releaseMapping(handle->mPteMemory)) {
        return status;
    }

    for (km::MemoryRange range : handle->mPhysicalMemory) {
        system->releaseMemory(range);
    }

    handle->mPhysicalMemory.clear();

    system->removeObject(process);

    return OsStatusSuccess;
}
