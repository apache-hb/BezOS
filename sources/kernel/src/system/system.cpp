#include "system/system.hpp"

#include "memory/address_space.hpp"
#include "memory/page_allocator.hpp"

static constexpr size_t kDefaultPtePageCount = 128;

void sys2::System::addObject(sm::RcuSharedPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    auto [it, ok] = mObjects.insert(object);
    KM_CHECK(ok, "Failed to add object to system");
}

void sys2::System::removeObject(sm::RcuWeakPtr<IObject> object) {
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
