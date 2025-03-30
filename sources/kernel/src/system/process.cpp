#include "system/process.hpp"
#include "memory.hpp"
#include "system/system.hpp"
#include "xsave.hpp"

sys2::ProcessHandle::ProcessHandle(sm::RcuWeakPtr<Process> process, OsHandle handle, ProcessAccess access)
    : mProcess(process)
    , mHandle(handle)
    , mAccess(access)
{ }

sm::RcuWeakPtr<sys2::IObject> sys2::ProcessHandle::getObject() {
    return mProcess;
}

OsStatus sys2::ProcessHandle::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    if (!hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    auto process = mProcess.lock();
    if (!process) {
        return OsStatusInvalidHandle;
    }

    return process->createProcess(system, info, handle);
}

OsStatus sys2::ProcessHandle::destroyProcess(System *system, const ProcessDestroyInfo& info) {
    if (!hasAccess(ProcessAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    auto process = mProcess.lock();

    if (!process) {
        return OsStatusInvalidHandle;
    }

    return process->destroy(system, info);
}

OsStatus sys2::ProcessHandle::createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle) {
    if (!hasAccess(ProcessAccess::eThreadControl)) {
        return OsStatusAccessDenied;
    }

    auto process = mProcess.lock();

    if (!process) {
        return OsStatusInvalidHandle;
    }

    return process->createThread(system, info, handle);
}

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

sys2::Process::Process(const ProcessCreateInfo& createInfo, sm::RcuWeakPtr<Process> parent, const km::AddressSpace *systemTables, km::AddressMapping pteMemory)
    : mName(createInfo.name)
    , mSupervisor(createInfo.supervisor)
    , mParent(parent)
    , mPteMemory(pteMemory)
    , mPageTables(systemTables, mPteMemory, km::PageFlags::eUserAll, km::DefaultUserArea())
{ }

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

OsStatus sys2::Process::createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (auto process = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, loanWeak(), system->pageTables(), pteMemory)) {
        ProcessHandle *result = new (std::nothrow) ProcessHandle(process, newHandleId(eOsHandleProcess), ProcessAccess::eAll);
        if (!result) {
            system->releaseMapping(pteMemory);
            return OsStatusOutOfMemory;
        }

        addHandle(result);
        addChild(process);

        system->addObject(process);
        *handle = result;
        return OsStatusSuccess;
    }

    system->releaseMapping(pteMemory);
    return OsStatusOutOfMemory;
}

OsStatus sys2::Process::destroy(System *system, const ProcessDestroyInfo& info) {
    stdx::UniqueLock guard(mLock);

    if (auto parent = mParent.lock()) {
        parent->removeChild(loanShared());
    }

    for (sm::RcuSharedPtr<Process> child : mChildren) {
        if (OsStatus status = child->destroy(system, ProcessDestroyInfo { .exitCode = info.exitCode, .reason = eOsProcessOrphaned })) {
            return status;
        }
    }

    for (sm::RcuSharedPtr<Thread> thread : mThreads) {
        if (OsStatus status = thread->destroy(system, ThreadDestroyInfo { .exitCode = info.exitCode, .reason = eOsThreadOrphaned })) {
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

    system->removeObject(loanWeak());

    return OsStatusSuccess;
}

OsStatus sys2::Process::createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle) {
    km::StackMapping kernelStack{};
    x64::XSave *fpuState = nullptr;
    ThreadHandle *result = nullptr;
    sm::RcuSharedPtr<Thread> thread;

    if (OsStatus status = system->mapSystemStack(&kernelStack)) {
        return status;
    }

    do {
        fpuState = km::CreateXSave();
        if (!fpuState) {
            break;
        }

        thread = sm::rcuMakeShared<sys2::Thread>(&system->rcuDomain(), info, loanWeak(), fpuState, kernelStack);
        if (!thread) {
            break;
        }

        result = new (std::nothrow) ThreadHandle(thread, newHandleId(eOsHandleThread), ThreadAccess::eAll);
        if (!result) {
            break;
        }

        addHandle(result);
        addThread(thread);

        system->addObject(thread);
        *handle = result;
        return OsStatusSuccess;
    } while (false);

    delete result;
    if (fpuState) km::DestroyXSave(fpuState);
    system->releaseStack(kernelStack);
    return OsStatusOutOfMemory;
}

OsStatus sys2::CreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **process) {
    km::AddressMapping pteMemory;
    if (OsStatus status = system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    if (auto root = sm::rcuMakeShared<sys2::Process>(&system->rcuDomain(), info, nullptr, system->pageTables(), pteMemory)) {
        ProcessHandle *result = new (std::nothrow) ProcessHandle(root, OS_HANDLE_NEW(eOsHandleProcess, 0), ProcessAccess::eAll);
        if (!result) {
            system->releaseMapping(pteMemory);
            return OsStatusOutOfMemory;
        }

        system->addObject(root);
        *process = result;
        return OsStatusSuccess;
    }

    system->releaseMapping(pteMemory);
    return OsStatusOutOfMemory;
}
