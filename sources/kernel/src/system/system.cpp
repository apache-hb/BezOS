#include "system/system.hpp"

#include "memory/address_space.hpp"
#include "memory/layout.hpp"
#include "memory/page_allocator.hpp"

#include "system/create.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"
#include "system/thread.hpp"

static constexpr size_t kDefaultPtePageCount = 64;
static constexpr size_t kDefaultKernelStackSize = 4;

void sys2::System::addObject(sm::RcuSharedPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
}

void sys2::System::removeObject(sm::RcuWeakPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
}

void sys2::System::addProcessObject(sm::RcuSharedPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.insert(object);
    mSchedule.addProcess(object);
}

void sys2::System::removeProcessObject(sm::RcuWeakPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.erase(object);
}

void sys2::System::addThreadObject(sm::RcuSharedPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
    mSchedule.addThread(object);
}

void sys2::System::removeThreadObject(sm::RcuWeakPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
}

OsStatus sys2::System::mapProcessPageTables(km::AddressMapping *mapping) {
    km::MemoryRange range = mPageAllocator->alloc4k(kDefaultPtePageCount);
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->map(range, km::PageFlags::eData, km::MemoryType::eWriteBack, mapping)) {
        mPageAllocator->release(range);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::System::mapSystemStack(km::StackMapping *mapping) {
    km::MemoryRange stack = mPageAllocator->alloc4k(kDefaultKernelStackSize);
    if (stack.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->mapStack(stack, km::PageFlags::eData, mapping)) {
        mPageAllocator->release(stack);
        return status;
    }

    return OsStatusSuccess;
}

void sys2::System::releaseMemory(km::MemoryRange range) {
    mPageAllocator->release(range);
}

OsStatus sys2::System::releaseMapping(km::AddressMapping mapping) {
    if (OsStatus status = mSystemTables->unmap(mapping.virtualRange())) {
        return status;
    }

    mPageAllocator->release(mapping.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys2::System::releaseStack(km::StackMapping mapping) {
    OsStatus status = mSystemTables->unmapStack(mapping);
    if (status != OsStatusSuccess) {
        return status;
    }

    mPageAllocator->release(mapping.stack.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys2::System::getProcessList(stdx::Vector2<sm::RcuSharedPtr<Process>>& list) {
    stdx::SharedLock guard(mLock);
    list.reserve(mProcessObjects.size());
    for (auto& process : mProcessObjects) {
        list.push_back(process);
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysCreateTx(InvokeContext *invoke, TxCreateInfo createInfo, TxHandle **handle) {
    sys2::ProcessHandle *parent = createInfo.process;
    sys2::ProcessHandle *invoker = invoke->process;

    if (!parent->hasAccess(ProcessAccess::eTxControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Tx> tx = sm::rcuMakeShared<Tx>(&invoke->system->rcuDomain(), createInfo);
    if (!tx) {
        return OsStatusOutOfMemory;
    }

    sm::RcuSharedPtr<Process> process = invoker->getProcess();

    OsHandle id = process->newHandleId(eOsHandleTx);
    TxHandle *result = new (std::nothrow) TxHandle(tx, id, TxAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    process->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysCommitTx(InvokeContext *, TxDestroyInfo) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysAbortTx(InvokeContext *, TxDestroyInfo) {
    return OsStatusNotSupported;
}

// mutex

OsStatus sys2::SysCreateMutex(InvokeContext *, MutexCreateInfo, MutexHandle **) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysDestroyMutex(InvokeContext *, MutexDestroyInfo) {
    return OsStatusNotSupported;
}
