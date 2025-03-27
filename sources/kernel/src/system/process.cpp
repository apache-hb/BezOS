#include "system/process.hpp"
#include "system/system.hpp"

void sys2::Process::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
    mName = name;
}

sys2::ObjectName sys2::Process::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

OsStatus sys2::Process::open(OsHandle*) {
    return OsStatusNotSupported;
}

OsStatus sys2::Process::close(OsHandle) {
    return OsStatusNotSupported;
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

sys2::Process::Process(const ProcessCreateInfo& createInfo, const km::PageTables *systemTables, km::AddressMapping pteMemory)
    : mName(createInfo.name)
    , mSupervisor(createInfo.supervisor)
    , mPageTables(systemTables->pageManager(), pteMemory, km::PageFlags::eUserAll)
    , mVmemAllocator(km::DefaultUserArea())
{
    km::copyHigherHalfMappings(&mPageTables, systemTables);
}

OsStatus sys2::CreateProcess(System *system, const ProcessCreateInfo& info, sm::RcuSharedPtr<Process> *process) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (auto result = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, system->pageTables(), pteMemory)) {
        system->addObject(result.weak());
        *process = result;
        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}
