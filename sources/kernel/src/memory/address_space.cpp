#include "memory/address_space.hpp"
#include "memory/stack_mapping.hpp"
#include "memory/tables.hpp"
#include "memory/allocator.hpp"
#include "panic.hpp"

void km::AddressSpace::reserve(VirtualRange range) {
    TlsfAllocation allocation;
    if (OsStatus status = reserve(range.cast<sm::VirtualAddress>(), &allocation)) {
        KmDebugMessage("[MEM] Failed to reserve virtual memory ", range, ": ", OsStatusId(status), "\n");
    }
}

OsStatus km::AddressSpace::reserve(VirtualRangeEx range, TlsfAllocation *result [[gnu::nonnull]]) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    TlsfAllocation allocation;

    if (OsStatus status = mVmemHeap.reserve(range.cast<km::PhysicalAddress>(), &allocation)) {
        return status;
    }

    *result = allocation;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::unmap(VirtualRange range) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    TlsfAllocation allocation;
    if (OsStatus status = mVmemHeap.findAllocation(std::bit_cast<PhysicalAddress>(range.front), &allocation)) {
        return status;
    }

    if (OsStatus status = mTables.unmap(range)) {
        return status;
    }

    mVmemHeap.free(allocation);

    return OsStatusSuccess;
}

void *km::AddressSpace::mapGenericObject(MemoryRangeEx range, PageFlags flags, MemoryType type, TlsfAllocation *allocation) {
    uintptr_t offset = (range.front.address & 0xFFF);
    MemoryRangeEx aligned = alignedOut(range, x64::kPageSize);
    if (aligned.isEmpty()) {
        KmDebugMessage("[MEM] Failed to map object memory ", range, ": empty range\n");
        return nullptr;
    }

    TlsfAllocation memory;
    if (OsStatus status = map(aligned, flags, type, &memory)) {
        KmDebugMessage("[MEM] Failed to map object memory ", aligned, ": ", OsStatusId(status), "\n");
        return nullptr;
    }

    KM_CHECK(!memory.isNull(), "Memory allocation is null");

    sm::VirtualAddress base = std::bit_cast<sm::VirtualAddress>(memory.address());
    *allocation = memory;
    return base + offset;
}

void *km::AddressSpace::map(MemoryRange range, PageFlags flags, MemoryType type) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return nullptr;
    }

    TlsfAllocation result;
    if (map(range.cast<km::PhysicalAddressEx>(), flags, type, &result) != OsStatusSuccess) {
        return nullptr;
    }

    return std::bit_cast<void*>(result.address());
}

OsStatus km::AddressSpace::map(MemoryRange range, const void *, PageFlags flags, MemoryType type, AddressMapping *mapping) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    TlsfAllocation vmem = mVmemHeap.aligned_alloc(x64::kPageSize, range.size());
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

    AddressMapping m = {
        .vaddr = std::bit_cast<const void*>(vmem.address()),
        .paddr = range.front.address,
        .size = range.size()
    };

    if (OsStatus status = mTables.map(m, flags, type)) {
        mVmemHeap.free(vmem);
        return status;
    }

    *mapping = m;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(MemoryRange range, PageFlags flags, StackMapping *mapping) {
    std::array<TlsfAllocation, 3> results;
    MemoryRangeEx stackRange = range.cast<km::PhysicalAddressEx>();
    if (OsStatus status = mapStack(stackRange, flags, &results)) {
        return status;
    }

    AddressMapping stack = {
        .vaddr = std::bit_cast<const void*>(results[1].address()),
        .paddr = stackRange.front.address,
        .size = stackRange.size()
    };

    StackMapping result = {
        .stack = stack,
        .total = km::merge(results[0].range(), results[2].range()).cast<const void*>(),
    };

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::unmapStack(StackMapping mapping) {
    stdx::LockGuard guard(mLock);

    std::array<TlsfAllocation, 3> results;
    if (OsStatus status = mVmemHeap.findAllocation(std::bit_cast<PhysicalAddress>(mapping.total.front), &results[0])) {
        KmDebugMessage("Failed to find stack allocation 0: ", mapping.total, ", ", mapping.stack, "\n");
        return status;
    }

    if (OsStatus status = mVmemHeap.findAllocation(std::bit_cast<PhysicalAddress>(mapping.stack.vaddr), &results[1])) {
        KmDebugMessage("Failed to find stack allocation 1\n");
        return status;
    }

    if (OsStatus status = mVmemHeap.findAllocation(std::bit_cast<PhysicalAddress>(mapping.total.back) - x64::kPageSize, &results[2])) {
        KmDebugMessage("Failed to find stack allocation 2\n");
        return status;
    }

    for (auto& result : results) {
        if (OsStatus status = mTables.unmap(result.range().cast<const void*>())) {
            KM_ASSERT(status == OsStatusSuccess);
        }
    }

    mVmemHeap.free(results[0]);
    mVmemHeap.free(results[1]);
    mVmemHeap.free(results[2]);

    return OsStatusSuccess;
}

void km::AddressSpace::updateHigherHalfMappings(const PageTables *source) {
    km::copyHigherHalfMappings(&mTables, source);
}

OsStatus km::AddressSpace::map(MemoryRangeEx memory, PageFlags flags, MemoryType type, TlsfAllocation *allocation) {
    if (memory.isEmpty() || (memory != alignedOut(memory, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    TlsfAllocation vmem = mVmemHeap.aligned_alloc(x64::kPageSize, memory.size());
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

    KM_CHECK(!vmem.isNull(), "Memory allocation is null");

    AddressMapping m = {
        .vaddr = std::bit_cast<const void*>(vmem.address()),
        .paddr = memory.front.address,
        .size = memory.size()
    };

    if (OsStatus status = mTables.map(m, flags, type)) {
        mVmemHeap.free(vmem);
        return status;
    }

    KM_CHECK(!vmem.isNull(), "Memory allocation is null");

    *allocation = vmem;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::map(TlsfAllocation memory, PageFlags flags, MemoryType type, MappingAllocation *allocation) {
    TlsfAllocation result;

    if (OsStatus status = map(memory.range().cast<km::PhysicalAddressEx>(), flags, type, &result)) {
        return status;
    }

    *allocation = MappingAllocation::unchecked(memory, result);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::map(AddressMapping mapping, PageFlags flags, MemoryType type, TlsfAllocation *allocation) {
    stdx::LockGuard guard(mLock);

    TlsfAllocation result;
    if (OsStatus status = mVmemHeap.reserve(mapping.virtualRange().cast<km::PhysicalAddress>(), &result)) {
        return status;
    }

    if (OsStatus status = mTables.map(mapping, flags, type)) {
        mVmemHeap.free(result);
        return status;
    }

    *allocation = result;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(TlsfAllocation memory, PageFlags flags, StackMappingAllocation *allocation) {
    std::array<TlsfAllocation, 3> results;
    MemoryRangeEx stackRange = memory.range().cast<km::PhysicalAddressEx>();
    if (OsStatus status = mapStack(stackRange, flags, &results)) {
        return status;
    }

    *allocation = StackMappingAllocation::unchecked(memory, results[1], results[0], results[2]);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(MemoryRangeEx memory, PageFlags flags, std::array<TlsfAllocation, 3> *allocations) {
    stdx::LockGuard guard(mLock);

    std::array<TlsfAllocation, 3> results;

    TlsfAllocation vmem = mVmemHeap.aligned_alloc(x64::kPageSize, memory.size() + (2 * x64::kPageSize));
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

    auto resultRange = vmem.range().cast<sm::VirtualAddress>();
    std::array<PhysicalAddress, 2> points = {
        std::bit_cast<km::PhysicalAddress>(resultRange.front + x64::kPageSize),
        std::bit_cast<km::PhysicalAddress>(resultRange.back - x64::kPageSize),
    };

    if (OsStatus status = mVmemHeap.splitv(vmem, points, results)) {
        OsStatus inner = unmap(vmem);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    AddressMapping m {
        .vaddr = std::bit_cast<const void*>(results[1].address()),
        .paddr = memory.front.address,
        .size = memory.size()
    };

    if (OsStatus status = mTables.map(m, flags, MemoryType::eWriteBack)) {
        OsStatus inner = unmap(vmem);
        KM_ASSERT(inner == OsStatusSuccess);
        for (auto& result : results) {
            mVmemHeap.free(result);
        }
        return status;
    }

    *allocations = results;

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::reserve(size_t size, TlsfAllocation *result [[gnu::nonnull]]) {
    if (size == 0 || size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);
    TlsfAllocation allocation = mVmemHeap.aligned_alloc(x64::kPageSize, size);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    *result = allocation;
    return OsStatusSuccess;
}

void km::AddressSpace::release(TlsfAllocation allocation) noexcept {
    stdx::LockGuard guard(mLock);
    mVmemHeap.free(allocation);
}

OsStatus km::AddressSpace::unmap(MappingAllocation allocation) {
    return unmap(allocation.virtualAllocation());
}

OsStatus km::AddressSpace::unmap(TlsfAllocation allocation) {
    if (OsStatus status = mTables.unmap(allocation.range().cast<const void*>())) {
        return status;
    }

    stdx::LockGuard guard(mLock);
    mVmemHeap.free(allocation);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[clang::noescape, gnu::nonnull]]) {
    if (OsStatus status = km::PageTables::create(pm, pteMemory, flags, &space->mTables)) {
        return status;
    }

    if (OsStatus status = km::TlsfHeap::create(vmem.cast<km::PhysicalAddress>(), &space->mVmemHeap)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const AddressSpace *source, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[clang::noescape, gnu::nonnull]]) {
    if (OsStatus status = km::AddressSpace::create(source->pageManager(), pteMemory, flags, vmem, space)) {
        return status;
    }

    space->updateHigherHalfMappings(source);
    return OsStatusSuccess;
}
