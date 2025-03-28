#include "system/system.hpp"

#include "memory/page_allocator.hpp"

static constexpr size_t kDefaultPtePageCount = 128;

void sys2::System::addObject(sm::RcuWeakPtr<IObject> object) {
    stdx::UniqueLock guard(mLock);
    mObjects.insert(object);
}

OsStatus sys2::System::mapProcessPageTables(km::AddressMapping *) {
    km::MemoryRange range = mPageAllocator->alloc4k(kDefaultPtePageCount);
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    KM_PANIC("unimplemented");
}
