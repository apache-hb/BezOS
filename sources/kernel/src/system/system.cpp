#include "system/system.hpp"

#include "fs/base.hpp"
#include "memory/address_space.hpp"
#include "memory/layout.hpp"
#include "memory/page_allocator.hpp"

#include "system/create.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"
#include "system/thread.hpp"
#include "system/pmm.hpp"

static constexpr size_t kDefaultPtePageCount = 64;
static constexpr size_t kDefaultKernelStackSize = 4;

OsStatus sys::System::addThreadObject(sm::RcuSharedPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
    return mSchedule.addThread(object);
}

void sys::System::removeThreadObject(sm::RcuWeakPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
    mSchedule.removeThread(object);
}

void sys::System::addProcessObject(sm::RcuSharedPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.insert(object);
}

void sys::System::removeProcessObject(sm::RcuWeakPtr<Process> object) {
    stdx::UniqueLock guard(mLock);
    mProcessObjects.erase(object);
}

OsStatus sys::System::mapProcessPageTables(km::AddressMapping *mapping) {
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

OsStatus sys::System::mapSystemStack(km::StackMapping *mapping) {
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

void sys::System::releaseMemory(km::MemoryRange range) {
    mPageAllocator->release(range);
}

OsStatus sys::System::releaseMapping(km::AddressMapping mapping) {
    if (OsStatus status = mSystemTables->unmap(mapping.virtualRange())) {
        return status;
    }

    mPageAllocator->release(mapping.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys::System::releaseStack(km::StackMapping mapping) {
    OsStatus status = mSystemTables->unmapStack(mapping);
    if (status != OsStatusSuccess) {
        return status;
    }

    mPageAllocator->release(mapping.stack.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys::System::getProcessList(stdx::Vector2<sm::RcuSharedPtr<Process>>& list) {
    stdx::SharedLock guard(mLock);
    list.reserve(mProcessObjects.size());
    for (auto& process : mProcessObjects) {
        list.push_back(process);
    }

    return OsStatusSuccess;
}

sys::SystemStats sys::System::stats() {
    stdx::SharedLock guard(mLock);
    SystemStats stats {
        .objects = mObjects.size(),
        .processes = mProcessObjects.size(),
    };

    return stats;
}

OsStatus sys::System::create(vfs::VfsRoot *vfsRoot, km::AddressSpace *addressSpace, km::PageAllocator *pageAllocator, System *system [[clang::noescape, gnu::nonnull]]) {
    system->mPageAllocator = pageAllocator;
    system->mSystemTables = addressSpace;
    system->mVfsRoot = vfsRoot;
    if (OsStatus status = sys::MemoryManager::create(system->mPageAllocator, &system->mMemoryManager)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::SysTxCreate(InvokeContext *context, TxCreateInfo info, OsTxHandle *handle) {
    sm::RcuSharedPtr<Tx> tx = sm::rcuMakeShared<Tx>(&context->system->rcuDomain(), info.name);
    if (!tx) {
        return OsStatusOutOfMemory;
    }

    OsHandle id = context->process->newHandleId(eOsHandleTx);
    TxHandle *result = new (std::nothrow) TxHandle(tx, id, TxAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    context->process->addHandle(result);
    *handle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysTxCommit(InvokeContext *, OsTxHandle) {
    return OsStatusNotSupported;
}

OsStatus sys::SysTxAbort(InvokeContext *, OsTxHandle) {
    return OsStatusNotSupported;
}

// mutex

OsStatus sys::SysMutexCreate(InvokeContext *, OsMutexCreateInfo, OsHandle *) {
    return OsStatusNotSupported;
}

OsStatus sys::SysMutexDestroy(InvokeContext *, OsHandle) {
    return OsStatusNotSupported;
}
