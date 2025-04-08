#include "system/system.hpp"

#include "memory/address_space.hpp"
#include "memory/page_allocator.hpp"

#include "system/process.hpp"

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
}

void sys2::System::removeProcessObject(sm::RcuWeakPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.erase(object);
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
    km::MemoryRange range = mPageAllocator->alloc4k(kDefaultKernelStackSize);
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->mapStack(range, km::PageFlags::eData, mapping)) {
        mPageAllocator->release(range);
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

    mPageAllocator->release(mapping.mapping.physicalRange());
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

OsStatus sys2::SysCreateProcess(InvokeContext *context, ProcessCreateInfo info, ProcessHandle **handle) {
    ProcessHandle *parent = info.process;
    TxHandle *tx = info.tx;
    km::AddressMapping pteMemory;
    sm::RcuSharedPtr<Process> process;
    ProcessHandle *result = nullptr;

    // Check that we can control the parent process.
    if (!parent->hasAccess(ProcessAccess::eProcessControl)) {
        return OsStatusAccessDenied;
    }

    // If there is a transaction, ensure we can append to it.
    if (tx && !tx->hasAccess(TxAccess::eWrite)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = context->system->mapProcessPageTables(&pteMemory)) {
        return status;
    }

    // Create the process.
    process = sm::rcuMakeShared<Process>(&context->system->rcuDomain(), info, parent->getProcess(), context->system->pageTables(), pteMemory);
    if (!process) {
        goto outOfMemory;
    }

    result = new (std::nothrow) ProcessHandle(process, context->process->getProcess()->newHandleId(eOsHandleProcess), ProcessAccess::eAll);
    if (!result) {
        goto outOfMemory;
    }

    // Add the process to the parent.
    parent->getProcess()->addChild(process);
    context->process->getProcess()->addHandle(result);
    context->system->addProcessObject(process);

    *handle = result;

    return OsStatusSuccess;

outOfMemory:
    delete result;
    process.reset();
    context->system->releaseMapping(pteMemory);
    return OsStatusOutOfMemory;
}

OsStatus sys2::SysDestroyProcess(InvokeContext *context, ProcessDestroyInfo info) {

}

OsStatus sys2::SysCreateThread(InvokeContext *context, ThreadCreateInfo info, ThreadHandle **handle) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysCreateTx(InvokeContext *context, TxCreateInfo info, TxHandle **handle) {
    return OsStatusNotSupported;
}

OsStatus sys2::SysCreateMutex(InvokeContext *context, MutexCreateInfo info, MutexHandle **handle) {
    return OsStatusNotSupported;
}
