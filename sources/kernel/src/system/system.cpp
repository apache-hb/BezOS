#include "system/system.hpp"

#include "fs/base.hpp"
#include "memory/address_space.hpp"
#include "memory/layout.hpp"
#include "memory/page_allocator.hpp"

#include "memory/stack_mapping.hpp"
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
    return object->attach(&mScheduler);
}

void sys::System::removeThreadObject(sm::RcuWeakPtr<Thread> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.erase(object);
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
    km::PmmAllocation allocation = mPageAllocator->pageAlloc(kDefaultPtePageCount);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->map(allocation.range(), km::PageFlags::eData, km::MemoryType::eWriteBack, mapping)) {
        mPageAllocator->free(allocation);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::System::mapSystemStack(km::StackMapping *mapping) {
    km::PmmAllocation allocation = mPageAllocator->pageAlloc(kDefaultKernelStackSize);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mSystemTables->mapStack(allocation.range(), km::PageFlags::eData, mapping)) {
        mPageAllocator->free(allocation);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::System::mapSystemStackEx(km::StackMappingAllocation *mapping) {
    km::PmmAllocation allocation = mPageAllocator->pageAlloc(kDefaultKernelStackSize);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    std::array<km::VmemAllocation, 3> results;

    if (OsStatus status = mSystemTables->mapStack(allocation.range().cast<sm::PhysicalAddress>(), km::PageFlags::eData, &results)) {
        mPageAllocator->free(allocation);
        return status;
    }

    *mapping = km::StackMappingAllocation::unchecked(allocation, results[1], results[0], results[2]);

    return OsStatusSuccess;
}

OsStatus sys::System::releaseMapping(km::AddressMapping mapping) {
    if (OsStatus status = mSystemTables->unmap(mapping.virtualRange())) {
        return status;
    }

    mPageAllocator->release(mapping.physicalRange());
    return OsStatusSuccess;
}

OsStatus sys::System::releaseStack(km::StackMappingAllocation mapping) {
    OsStatus status = mSystemTables->unmap(mapping.stackAllocation());
    KM_ASSERT(status == OsStatusSuccess);

    if (mapping.hasGuardHead()) {
        status = mSystemTables->unmap(mapping.guardHead());
        KM_ASSERT(status == OsStatusSuccess);
    }

    if (mapping.hasGuardTail()) {
        status = mSystemTables->unmap(mapping.guardTail());
        KM_ASSERT(status == OsStatusSuccess);
    }

    mPageAllocator->free(mapping.memoryAllocation());
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
