#include "memory.hpp"

#include "logger/categories.hpp"
#include "memory/allocator.hpp"
#include "memory/memory.hpp"
#include "memory/stack_mapping.hpp"

void *km::SystemMemory::allocate(size_t size, PageFlags flags, MemoryType type) {
    MappingAllocation allocation;

    if (map(size, flags, type, &allocation) != OsStatusSuccess) {
        return nullptr;
    }

    return allocation.baseAddress();
}

km::AddressMapping km::SystemMemory::allocateStack(size_t size) {
    StackMappingAllocation stackMapping;
    if (mapStack(size, PageFlags::eData, &stackMapping) != OsStatusSuccess) {
        return AddressMapping{};
    }

    AddressMapping mapping {
        .vaddr = stackMapping.baseAddress(),
        .paddr = std::bit_cast<km::PhysicalAddress>(stackMapping.baseMemory()),
        .size = stackMapping.stackSize(),
    };

    return mapping;
}


void km::SystemMemory::reserve(AddressMapping mapping) {
    reserveVirtual(mapping.virtualRange());
    reservePhysical(mapping.physicalRange());
}

void km::SystemMemory::reservePhysical(MemoryRange range) {
    TlsfAllocation allocation;
    if (OsStatus status = mPageAllocator.reserve(range, &allocation)) {
        MemLog.warnf("Failed to reserve physical memory ", range, ": ", OsStatusId(status));
        return;
    }
}

void km::SystemMemory::reserveVirtual(VirtualRange range) {
    mTables.reserve(range);
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

    VmemAllocation memory;
    if (mTables.map(range.cast<km::PhysicalAddressEx>(), flags, type, &memory) != OsStatusSuccess) {
        return nullptr;
    }

    sm::VirtualAddress base = std::bit_cast<sm::VirtualAddress>(memory.address());
    return base + offset;
}

OsStatus km::SystemMemory::mapStack(size_t size, PageFlags flags, StackMappingAllocation *mapping) {
    if (size == 0 || size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    TlsfAllocation memory = mPageAllocator.pageAlloc(Pages(size));
    if (memory.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mTables.mapStack(memory, flags, mapping)) {
        mPageAllocator.free(memory);
        return status;
    }

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
        mPageAllocator.free(memory);
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::SystemMemory::map(MemoryRangeEx memory, PageFlags flags, MemoryType type, VmemAllocation *allocation) {
    return mTables.map(memory, flags, type, allocation);
}

OsStatus km::SystemMemory::unmap(MappingAllocation allocation) {
    if (OsStatus status = mTables.unmap(allocation)) {
        return status;
    }

    mPageAllocator.free(allocation.memoryAllocation());
    return OsStatusSuccess;
}

OsStatus km::SystemMemory::create(std::span<const boot::MemoryRegion> memmap, VirtualRangeEx systemArea, PageBuilder pm, AddressMapping pteMemory, SystemMemory *system [[clang::noescape, gnu::nonnull]]) {
    system->mPageManager = pm;

    if (OsStatus status = km::AddressSpace::create(&system->mPageManager, pteMemory, PageFlags::eAll, systemArea, &system->mTables)) {
        return status;
    }

    if (OsStatus status = km::PageAllocator::create(memmap, &system->mPageAllocator)) {
        return status;
    }

    return OsStatusSuccess;
}
