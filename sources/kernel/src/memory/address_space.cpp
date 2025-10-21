#include "memory/address_space.hpp"
#include "logger/categories.hpp"
#include "memory/stack_mapping.hpp"
#include "memory/tables.hpp"
#include "memory/allocator.hpp"
#include "panic.hpp"

void km::AddressSpace::reserve(VirtualRange range) {
    VmemAllocation allocation;
    if (OsStatus status = reserve(range.cast<sm::VirtualAddress>(), &allocation)) {
        MemLog.warnf("Failed to reserve virtual memory ", range, ": ", OsStatusId(status));
    }
}

OsStatus km::AddressSpace::reserve(VirtualRangeEx range, VmemAllocation *result [[outparam]]) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    return mVmemHeap.reserve(range, result);
}

OsStatus km::AddressSpace::unmap(VirtualRange range) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    VmemAllocation allocation;
    if (OsStatus status = mVmemHeap.findAllocation(range.front, &allocation)) {
        return status;
    }

    if (OsStatus status = mTables.unmap(range)) {
        return status;
    }

    mVmemHeap.free(allocation);

    return OsStatusSuccess;
}

void *km::AddressSpace::mapGenericObject(MemoryRangeEx range, PageFlags flags, MemoryType type, VmemAllocation *allocation [[outparam]]) {
    uintptr_t offset = (range.front.address & 0xFFF);
    MemoryRangeEx aligned = alignedOut(range, x64::kPageSize);
    if (aligned.isEmpty()) {
        MemLog.warnf("Failed to map object memory ", range, ": empty range");
        return nullptr;
    }

    VmemAllocation memory;
    if (OsStatus status = map(aligned, flags, type, &memory)) {
        MemLog.warnf("Failed to map object memory ", aligned, ": ", OsStatusId(status));
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

    VmemAllocation result;
    if (map(range.cast<km::PhysicalAddressEx>(), flags, type, &result) != OsStatusSuccess) {
        return nullptr;
    }

    return std::bit_cast<void*>(result.address());
}

OsStatus km::AddressSpace::map(MemoryRange range, const void *, PageFlags flags, MemoryType type, AddressMapping *mapping [[outparam]]) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    VmemAllocation vmem = mVmemHeap.alignedAlloc(x64::kPageSize, range.size());
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

OsStatus km::AddressSpace::mapStack(MemoryRange range, PageFlags flags, StackMapping *mapping [[outparam]]) {
    std::array<VmemAllocation, 3> results;
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

    std::array<VmemAllocation, 3> results;
    if (OsStatus status = mVmemHeap.findAllocation(mapping.total.front, &results[0])) {
        MemLog.warnf("Failed to find stack allocation 0: ", mapping.total, ", ", mapping.stack);
        return status;
    }

    if (OsStatus status = mVmemHeap.findAllocation(mapping.stack.vaddr, &results[1])) {
        MemLog.warnf("Failed to find stack allocation 1", OsStatusId(status));
        return status;
    }

    if (OsStatus status = mVmemHeap.findAllocation(std::bit_cast<sm::VirtualAddress>(mapping.total.back) - x64::kPageSize, &results[2])) {
        MemLog.warnf("Failed to find stack allocation 2 ", OsStatusId(status));
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

OsStatus km::AddressSpace::map(MemoryRangeEx memory, PageFlags flags, MemoryType type, VmemAllocation *allocation [[outparam]]) {
    if (memory.isEmpty() || (memory != alignedOut(memory, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    VmemAllocation vmem = mVmemHeap.alignedAlloc(x64::kPageSize, memory.size());
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

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

OsStatus km::AddressSpace::map(PmmAllocation memory, PageFlags flags, MemoryType type, MappingAllocation *allocation [[outparam]]) {
    VmemAllocation result;

    if (OsStatus status = map(memory.range().cast<km::PhysicalAddressEx>(), flags, type, &result)) {
        return status;
    }

    *allocation = MappingAllocation::unchecked(memory, result);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(PmmAllocation memory, PageFlags flags, StackMappingAllocation *allocation [[outparam]]) {
    std::array<VmemAllocation, 3> results;
    MemoryRangeEx stackRange = memory.range().cast<km::PhysicalAddressEx>();
    if (OsStatus status = mapStack(stackRange, flags, &results)) {
        return status;
    }

    *allocation = StackMappingAllocation::unchecked(memory, results[1], results[0], results[2]);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(MemoryRangeEx memory, PageFlags flags, std::array<VmemAllocation, 3> *allocations [[outparam]]) {
    stdx::LockGuard guard(mLock);

    std::array<VmemAllocation, 3> results;

    VmemAllocation vmem = mVmemHeap.alignedAlloc(x64::kPageSize, memory.size() + (2 * x64::kPageSize));
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

    auto resultRange = vmem.range().cast<sm::VirtualAddress>();
    std::array<sm::VirtualAddress, 2> points = {
        resultRange.front + x64::kPageSize,
        resultRange.back - x64::kPageSize,
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

OsStatus km::AddressSpace::unmap(MappingAllocation allocation) {
    return unmap(VmemAllocation{allocation.virtualAllocation().getBlock()});
}

OsStatus km::AddressSpace::unmap(VmemAllocation allocation) {
    if (OsStatus status = mTables.unmap(allocation.range().cast<const void*>())) {
        return status;
    }

    stdx::LockGuard guard(mLock);
    mVmemHeap.free(VmemAllocation{allocation.getBlock()});
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[outparam]]) {
    if (OsStatus status = km::PageTables::create(pm, pteMemory, flags, &space->mTables)) {
        return status;
    }

    if (OsStatus status = VmemHeap::create(vmem, &space->mVmemHeap)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const AddressSpace *source, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[outparam]]) {
    if (OsStatus status = km::AddressSpace::create(source->pageManager(), pteMemory, flags, vmem, space)) {
        return status;
    }

    space->updateHigherHalfMappings(source);
    return OsStatusSuccess;
}
