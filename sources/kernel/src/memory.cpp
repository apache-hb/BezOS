#include "memory.hpp"

#include "memory/allocator.hpp"
#include "memory/memory.hpp"
#include "memory/stack_mapping.hpp"

km::SystemMemory::SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory)
    : mPageManager(pm)
    , mPageAllocator(memmap)
    , mTables(&mPageManager, pteMemory, PageFlags::eAll, systemArea)
{ }

void *km::SystemMemory::allocate(size_t size, PageFlags flags, MemoryType type) {
    return (void*)allocate(AllocateRequest {
        .size = size,
        .flags = flags,
        .type = type
    }).vaddr;
}

km::AddressMapping km::SystemMemory::allocateStack(size_t size) {
    size_t pages = Pages(size);

    MemoryRange range = mPageAllocator.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    StackMapping mapping;
    if (mTables.mapStack(range, PageFlags::eData, &mapping) != OsStatusSuccess) {
        mPageAllocator.release(range);
        return AddressMapping{};
    }

    return mapping.stack;
}

void km::SystemMemory::reserve(AddressMapping mapping) {
    reserveVirtual(mapping.virtualRange());
    reservePhysical(mapping.physicalRange());
}

void km::SystemMemory::reservePhysical(MemoryRange range) {
    mPageAllocator.reserve(range);
}

void km::SystemMemory::reserveVirtual(VirtualRange range) {
    mTables.reserve(range);
}

km::AddressMapping km::SystemMemory::allocate(AllocateRequest request) {
    size_t pages = Pages(request.size);

    MemoryRange range = mPageAllocator.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    AddressMapping mapping{};
    OsStatus status = mTables.map(range, request.flags, request.type, &mapping);
    if (status != OsStatusSuccess) {
        mPageAllocator.release(range);
        return AddressMapping{};
    }

    return mapping;
}

OsStatus km::SystemMemory::unmap(void *ptr, size_t size) {
    return unmap(VirtualRange::of(ptr, size));
}

OsStatus km::SystemMemory::unmap(VirtualRange range) {
    return mTables.unmap(range);
}

void *km::SystemMemory::map(MemoryRange range, PageFlags flags, MemoryType type) {
    //
    // I may be asked to map a range that is not page aligned
    // so I need to save the offset to apply to the virtual address before giving it back.
    //
    uintptr_t offset = (range.front.address & 0xFFF);

    MemoryRange aligned = PageAligned(range);

    reservePhysical(aligned);

    AddressMapping mapping{};
    OsStatus status = mTables.map(aligned, flags, type, &mapping);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return (void*)((uintptr_t)mapping.vaddr + offset);
}

OsStatus km::SystemMemory::mapStack(size_t size, PageFlags flags, StackMappingAllocation *mapping) {
    if (size == 0 || size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    TlsfAllocation memory = mPageAllocator.pageAlloc(Pages(size));
    if (memory.isNull()) {
        return OsStatusOutOfMemory;
    }

    TlsfAllocation stackMemory;
    if (OsStatus status = mTables.reserve(size + (2 * x64::kPageSize), &stackMemory)) {
        mPageAllocator.release(memory);
        return status;
    }

    MemoryRange stackRange = stackMemory.range();

    std::array<TlsfAllocation, 3> allocations;
    std::array<PhysicalAddress, 2> points = {
        stackRange.front + x64::kPageSize,
        stackRange.back - x64::kPageSize,
    };

    if (OsStatus status = mTables.splitv(stackMemory, points, allocations)) {
        mPageAllocator.release(memory);
        mTables.release(stackMemory);
        return status;
    }

    MappingAllocation stackMapping;

    if (OsStatus status = mTables.map(allocations[1], flags, MemoryType::eWriteBack, &stackMapping)) {
        mPageAllocator.release(memory);
        for (auto &alloc : allocations) {
            mTables.release(alloc);
        }
        return status;
    }

    *mapping = StackMappingAllocation::unchecked(stackMemory, allocations[1], allocations[0], allocations[2]);
    return OsStatusSuccess;
}

OsStatus km::SystemMemory::map(size_t size, PageFlags flags, MemoryType type, MappingAllocation *allocation) {
    if (size == 0 || size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    TlsfAllocation memory = mPageAllocator.pageAlloc(Pages(size));
    if (memory.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mTables.map(memory, flags, type, allocation)) {
        mPageAllocator.release(memory);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::SystemMemory::unmap(MappingAllocation allocation) {
    if (OsStatus status = mTables.unmap(allocation)) {
        return status;
    }

    mPageAllocator.release(allocation.memoryAllocation());
    return OsStatusSuccess;
}

OsStatus km::SystemMemory::create(std::span<const boot::MemoryRegion> memmap, VirtualRangeEx systemArea, PageBuilder pm, AddressMapping pteMemory, SystemMemory *system) {
    system->mPageManager = pm;

    if (OsStatus status = km::AddressSpace::create(&system->mPageManager, pteMemory, PageFlags::eAll, systemArea, &system->mTables)) {
        return status;
    }

    if (OsStatus status = km::PageAllocator::create(memmap, &system->mPageAllocator)) {
        return status;
    }

    return OsStatusSuccess;
}
