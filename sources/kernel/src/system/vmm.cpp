#include "system/vmm.hpp"
#include "memory/address_space.hpp"
#include "memory/tables.hpp"
#include "system/pmm.hpp"

#include "common/util/defer.hpp"

using sys::AddressSpaceManager;
using sys::detail::AddressSegment;

OsStatus AddressSpaceManager::map(MemoryManager *manager, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::VmemAllocation allocation = mHeap.alignedAlloc(align, size);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    km::MemoryRange range;
    if (OsStatus status = manager->allocate(size, align, &range)) {
        mHeap.free(allocation);
        return status;
    }

    km::AddressMapping result {
        .vaddr = std::bit_cast<const void*>(allocation.address()),
        .paddr = range.front,
        .size = size,
    };

    if (OsStatus status = mPageTables.map(result, flags, type)) {
        mHeap.free(allocation);
        OsStatus inner = manager->release(range);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    MemLog.infof("Mapped physical memory: ", result);

    auto segment = AddressSegment { range.cast<km::PhysicalAddressEx>(), allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::map(MemoryManager *manager, km::MemoryRange range, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::VmemAllocation allocation = mHeap.alignedAlloc(x64::kPageSize, range.size());
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    km::AddressMapping result {
        .vaddr = std::bit_cast<const void*>(allocation.address()),
        .paddr = range.front,
        .size = range.size(),
    };

    if (OsStatus status = mPageTables.map(result, flags, type)) {
        mHeap.free(allocation);
        OsStatus inner = manager->release(range);
        KM_ASSERT(inner == OsStatusSuccess);
        return status;
    }

    MemLog.infof("Mapped physical memory: ", result);

    auto segment = AddressSegment { range.cast<km::PhysicalAddressEx>(), allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::mapSegment(
    MemoryManager *manager, Iterator it,
    km::VirtualRange src, km::VirtualRange dst,
    km::PageFlags flags, km::MemoryType type,
    km::VmemAllocation allocation
) [[clang::allocating]] {
    OsStatus status = OsStatusSuccess;
    auto& segment = it->second;
    auto segmentMapping = segment.mapping();
    km::VirtualRange seg = segmentMapping.virtualRange();

    km::VirtualRange subrange = km::intersection(seg, src);
    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    auto backing = segment.getBackingMemory();
    km::MemoryRangeEx memory = { backing.front + frontOffset, backing.back - backOffset };

    size_t totalFrontOffset = (uintptr_t)subrange.front - (uintptr_t)src.front;
    size_t totalBackOffset = (uintptr_t)src.back - (uintptr_t)subrange.back;
    km::VirtualRange dstRange = { (void*)((uintptr_t)dst.front + totalFrontOffset), (void*)((uintptr_t)dst.back - totalBackOffset) };

    km::AddressMapping mapping {
        .vaddr = dstRange.front,
        .paddr = memory.front.address,
        .size = memory.size(),
    };

    status = manager->retain(memory.cast<km::PhysicalAddress>());
    if (status != OsStatusSuccess) {
        MemLog.warnf("[VMM] Failed to retain memory: ", memory, " ", seg, " ", src, " ", OsStatusId(status), "\n");
        KM_ASSERT(status == OsStatusSuccess);
        return status;
    }

    status = mPageTables.map(mapping, flags, type);
    KM_ASSERT(status == OsStatusSuccess);

    if (seg == src) {
        // |--------seg-------|
        // |--------src-------|
        AddressSegment newSegment { memory, allocation };
        addSegment(std::move(newSegment));
        return OsStatusCompleted;
    } else if (seg.contains(src)) {
        // |--------seg-------|
        //       |--src--|
        auto [lhs, rhs] = km::split(seg, src);
        KM_ASSERT(!lhs.isEmpty() && !rhs.isEmpty());

        auto allocRange = allocation.range();

        km::VmemAllocation lo, mid, hi;
        if (OsStatus status = mHeap.split(allocation, allocRange.front + frontOffset, &lo, &mid)) {
            return status;
        }

        if (OsStatus status = mHeap.split(mid, allocRange.back - backOffset, &mid, &hi)) {
            return status;
        }

        auto midSegment = AddressSegment { memory, mid };
        addNoAccessSegment(lo);
        addNoAccessSegment(hi);
        addSegment(std::move(midSegment));
        return OsStatusCompleted;
    } else {
        KM_PANIC("Unsupported mapping");
    }

    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::map(MemoryManager *manager, AddressSpaceManager *other, km::VirtualRange range, km::PageFlags flags, km::MemoryType type, km::VirtualRange *result [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    stdx::LockGuard guard2(other->mLock); // TODO: this can deadlock

    auto [begin, end] = other->mTable.find(range);

    if (begin == other->mTable.end()) {
        return OsStatusNotFound;
    }

    km::VmemAllocation allocation = mHeap.alignedAlloc(alignof(x64::page), range.size());
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (begin == end) {
        const auto& segment = begin->second;
        auto seg = segment.range();
        if (seg == range) {
            OsStatus status = OsStatusSuccess;
            km::MemoryRangeEx memory = segment.getBackingMemory();
            km::AddressMapping mapping {
                .vaddr = std::bit_cast<const void*>(allocation.address()),
                .paddr = memory.front.address,
                .size = memory.size(),
            };

            status = manager->retain(memory.cast<km::PhysicalAddress>());
            if (status != OsStatusSuccess) {
                MemLog.fatalf("Failed to retain memory: ", memory, " ", seg, " ", range, " ", OsStatusId(status));
                KM_ASSERT(status == OsStatusSuccess);
            }

            status = mPageTables.map(mapping, flags, type);
            if (status != OsStatusSuccess) {
                MemLog.fatalf("Failed to map memory: ", mapping, " ", seg, " ", range, " ", OsStatusId(status));
                KM_ASSERT(status == OsStatusSuccess);
            }

            AddressSegment newSegment { memory, allocation };
            addSegment(std::move(newSegment));
            *result = mapping.virtualRange();
            return OsStatusSuccess;
        } else if (seg.back == range.back) {
            //
            // Map the high part of a segment
            //
            OsStatus status = OsStatusSuccess;
            size_t offset = (uintptr_t)range.front - (uintptr_t)seg.front;
            auto backing = segment.getBackingMemory();
            km::MemoryRangeEx memory = { backing.front + offset, backing.back };
            km::AddressMapping mapping {
                .vaddr = std::bit_cast<const void*>(allocation.address()),
                .paddr = memory.front.address,
                .size = memory.size(),
            };

            status = manager->retain(memory.cast<km::PhysicalAddress>());
            if (status != OsStatusSuccess) {
                MemLog.fatalf("Failed to retain memory: ", memory, " ", seg, " ", range, " ", OsStatusId(status));
                KM_ASSERT(status == OsStatusSuccess);
            }

            status = mPageTables.map(mapping, flags, type);
            if (status != OsStatusSuccess) {
                MemLog.fatalf("Failed to map memory: ", mapping, " ", seg, " ", range, " ", OsStatusId(status));
                KM_ASSERT(status == OsStatusSuccess);
            }

            //
            // TODO: this retains the low part of the segment, we should split it instead
            //

            AddressSegment newSegment { memory, allocation };
            addSegment(std::move(newSegment));
            *result = mapping.virtualRange();
            return OsStatusSuccess;
        } else {
            MemLog.fatalf("Unsupported mapping: ", seg, " ", range);
            KM_ASSERT(false);
        }
    } else if (end != mTable.end()) {
        MemLog.fatalf("Unsupported mapping: ", begin->second.range(), " ", end->second.range(), " ", range);
        KM_ASSERT(false);
    }

    km::VirtualRange dst = allocation.range().cast<const void*>();

    for (auto it = begin; it != end; ++it) {
        switch (OsStatus status = mapSegment(manager, it, range, dst, flags, type, allocation)) {
        case OsStatusCompleted:
            *result = dst;
            return OsStatusSuccess;
        case OsStatusSuccess:
            continue;
        default:
            mHeap.free(allocation);
            return status;
        }
    }

    *result = dst;
    return OsStatusSuccess;
}

void AddressSpaceManager::addSegment(AddressSegment &&segment) noexcept [[clang::allocating]] {
    mTable.insert(std::move(segment));
}

void AddressSpaceManager::addNoAccessSegment(km::VmemAllocation allocation) noexcept [[clang::allocating]] {
    auto segment = AddressSegment { km::MemoryRangeEx{}, allocation };
    addSegment(std::move(segment));
}

OsStatus AddressSpaceManager::splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]] {
    auto& segment = it->second;

    km::MemoryRangeEx memory = segment.getBackingMemory();
    auto seg = segment.range();
    size_t frontOffset = (uintptr_t)midpoint - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)midpoint;

    km::VmemAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.getVmemAllocation(), std::bit_cast<uintptr_t>(midpoint), &lo, &hi)) {
        return status;
    }

    km::MemoryRangeEx loRange = { memory.front, memory.front + frontOffset };
    km::MemoryRangeEx hiRange = { memory.back - backOffset, memory.back };

    auto loSegment = AddressSegment { loRange, lo };
    auto hiSegment = AddressSegment { hiRange, hi };

    mTable.erase(it);

    switch (side) {
    case kLow:
        addSegment(std::move(hiSegment));
        deleteSegment(manager, std::move(loSegment));
        break;
    case kHigh:
        addSegment(std::move(loSegment));
        deleteSegment(manager, std::move(hiSegment));
        break;
    default:
        KM_PANIC("Invalid side");
        break;
    }

    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::unmapSegment(MemoryManager *manager, Iterator it, km::VirtualRange range, km::VirtualRange *remaining) [[clang::allocating]] {
    auto& segment = it->second;
    OsStatus status = OsStatusSuccess;
    km::VirtualRange seg = segment.range();

    km::VirtualRange subrange = km::intersection(seg, range);
    if (subrange.isEmpty()) {
        return OsStatusCompleted;
    }

    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    auto backing = segment.getBackingMemory();
    km::MemoryRangeEx srcMemory = { backing.front + frontOffset, backing.back - backOffset };
    if (range.contains(seg)) {
        // |-----seg-----|
        // |--------range-----|
        //
        //      |-----seg-----|
        // |--------range-----|
        //
        //    |-----seg-----|
        // |--------range-----|
        status = manager->release(srcMemory.cast<km::PhysicalAddress>());
        KM_ASSERT(status == OsStatusSuccess);

        status = mPageTables.unmap(seg);
        KM_ASSERT(status == OsStatusSuccess);

        mHeap.free(segment.getVmemAllocation());

        mTable.erase(it);
        *remaining = { seg.back, range.back };
        return OsStatusSuccess;
    } else {
        if (seg.front > range.front) {
            //       |--------seg-------|
            // |--------range-----|
            if (OsStatus status = splitSegment(manager, it, range.back, kLow)) {
                return status;
            }

            return OsStatusCompleted;
        } else {
            KM_ASSERT(range.back > seg.back && seg.contains(range.front));
            // |--------seg-------|
            //          |----range----|
            if (OsStatus status = splitSegment(manager, it, range.front, kHigh)) {
                return status;
            }

            return OsStatusSuccess;
        }
    }
}

OsStatus AddressSpaceManager::splitAtAddress(MemoryManager *manager, sm::VirtualAddress address) [[clang::allocating]] {
    KM_PANIC("Not implemented");
}

OsStatus AddressSpaceManager::map(MemoryManager *manager, sm::VirtualAddress address, km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]] {
    // TODO: on the error case memory is a bit busted
    km::VirtualRange range = { address, address + memory.size() };
    if (OsStatus status = unmap(manager, range)) {
        if (status != OsStatusNotFound) {
            return status;
        }
    }

    km::VmemAllocation allocation;

    stdx::LockGuard guard(mLock);
    if (OsStatus status = mHeap.reserve(range.cast<sm::VirtualAddress>(), &allocation)) {
        return status;
    }

    km::AddressMapping result {
        .vaddr = address,
        .paddr = memory.front,
        .size = memory.size(),
    };

    if (OsStatus status = mPageTables.map(result, flags, type)) {
        KM_ASSERT(status == OsStatusSuccess);
        return status;
    }

    auto segment = AddressSegment { memory.cast<km::PhysicalAddressEx>(), allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::mapExternal(km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::VmemAllocation allocation = mHeap.alignedAlloc(alignof(x64::page), memory.size());

    km::AddressMapping result {
        .vaddr = std::bit_cast<const void*>(allocation.address()),
        .paddr = memory.front,
        .size = memory.size(),
    };

    if (OsStatus status = mPageTables.map(result, flags, type)) {
        KM_ASSERT(status == OsStatusSuccess);
        return status;
    }

    auto segment = AddressSegment { km::MemoryRangeEx{}, allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::allocateVirtual(km::MemoryRange memory, sm::VirtualAddress address, size_t size, size_t align, bool addressIsHint, km::PageFlags flags, km::AddressMapping *result [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::VmemAllocation allocation;

    if (addressIsHint) {
        KM_ASSERT(!address.isNull());
        allocation = mHeap.allocateWithHint(align, size, address.address);
        if (allocation.isNull()) {
            return OsStatusOutOfMemory;
        }
    } else if (!address.isNull()) {
        if (OsStatus status = mHeap.reserve({ address.address, address.address + size }, &allocation)) {
            SysLog.infof("Failed to reserve memory: ", OsStatusId(status));
            mHeap.dump();
            return status;
        }
    } else {
        allocation = mHeap.alignedAlloc(align, size);
        if (allocation.isNull()) {
            return OsStatusOutOfMemory;
        }
    }

    km::AddressMapping mapping {
        .vaddr = std::bit_cast<const void*>(allocation.address()),
        .paddr = memory.front,
        .size = size,
    };

    if (OsStatus status = mPageTables.map(mapping, flags, km::MemoryType::eWriteBack)) {
        mHeap.free(allocation);
        return status;
    }

    auto segment = AddressSegment { memory.cast<km::PhysicalAddressEx>(), allocation };
    addSegment(std::move(segment));

    *result = mapping;
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::map(const AddressSpaceMappingRequest& request, km::AddressMapping *result [[outparam]]) {
    KM_PANIC("Not implemented");
}

void AddressSpaceManager::deleteSegment(MemoryManager *manager, AddressSegment&& segment) noexcept [[clang::allocating]] {
    OsStatus status = OsStatusSuccess;

    if (segment.hasBackingMemory()) {
        status = manager->release(segment.getBackingMemory().cast<km::PhysicalAddress>());
        KM_ASSERT(status == OsStatusSuccess);
    }

    status = mPageTables.unmap(segment.range());
    KM_ASSERT(status == OsStatusSuccess);

    mHeap.free(segment.getVmemAllocation());
}

AddressSpaceManager::Iterator AddressSpaceManager::eraseSegment(MemoryManager *manager, Iterator it) noexcept [[clang::allocating]] {
    auto& segment = it->second;

    deleteSegment(manager, std::move(segment));

    return mTable.erase(it);
}

void AddressSpaceManager::eraseSegment(MemoryManager *manager, km::VirtualRange segment) noexcept [[clang::allocating]] {
    auto it = segments().find(segment.back);
    KM_ASSERT(it != segments().end());
    eraseSegment(manager, it);
}

void AddressSpaceManager::eraseMany(MemoryManager *manager, km::VirtualRange range) noexcept [[clang::allocating]] {
    auto begin = segments().upper_bound(range.front);
    auto end = segments().upper_bound(range.back);

    for (auto it = begin; it != end;) {
        auto& segment = it->second;
        KM_ASSERT(range.contains(segment.range()));
        it = eraseSegment(manager, it);
        end = segments().upper_bound(range.back);
    }
}

OsStatus AddressSpaceManager::unmap(MemoryManager *manager, km::VirtualRange range) [[clang::allocating]] {
    KM_ASSERT(range.isValid());

    stdx::LockGuard guard(mLock);

    MemLog.infof("Unmapping range: ", range);

    defer {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety-analysis"); // The lock is held, but the compiler doesn't know it

        auto& s = segments();
        auto upper = s.upper_bound(range.front);
        if (upper != s.end()) {
            auto inner = upper->second.range();
            auto intersect = km::intersection(range, inner);
            if (!intersect.isEmpty()) {
                MemLog.fatalf("Failed to unmap: ", range, ", ", inner, ", ", intersect);
                KM_PANIC("internal error");
            }
        }

        CLANG_DIAGNOSTIC_POP();
    };

    auto [begin, end] = mTable.find(range);

    if (begin == mTable.end()) {
        return OsStatusNotFound;
    }

    //     |--------seg-------|
    // |--------range-----|

    if (begin == end) {
        auto& segment = begin->second;
        auto mapping = segment.mapping();
        auto seg = mapping.virtualRange();
        if (seg == range || range.contains(seg)) {
            // |--------seg-------|
            // |--------range-----|
            eraseSegment(manager, begin);
            return OsStatusSuccess;
        } else if (range.front == seg.front) {
            // |--------seg-------|
            // |---range---|
            return splitSegment(manager, begin, range.back, kLow);
        } else if (range.back == seg.back) {
            // |--------seg-------|
            //        |---range---|
            return splitSegment(manager, begin, range.front, kHigh);
        } else if (seg.contains(range)) {
            // |--------seg-------|
            //     |---range---|
            OsStatus status = OsStatusSuccess;

            auto [lhs, rhs] = km::split(seg, range);
            KM_ASSERT(!lhs.isEmpty() && !rhs.isEmpty());

            km::AddressMapping subrange = mapping.subrange(range);

            std::array<km::VmemAllocation, 3> allocations;
            std::array<sm::VirtualAddress, 2> points = {
                (uintptr_t)range.front,
                (uintptr_t)range.back,
            };

            if (OsStatus status = mHeap.splitv(segment.getVmemAllocation(), points, allocations)) {
                return status;
            }

            status = manager->release(subrange.physicalRange());
            KM_ASSERT(status == OsStatusSuccess);

            status = mPageTables.unmap(subrange.virtualRange());
            KM_ASSERT(status == OsStatusSuccess);

            auto backing = segment.getBackingMemory();
            AddressSegment loSegment { backing.first(lhs.size()), allocations[0] };
            AddressSegment hiSegment { backing.last(rhs.size()), allocations[2] };

            mHeap.free(allocations[1]);

            mTable.erase(begin);

            addSegment(std::move(loSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        } else if (seg.front > range.front && seg.back > range.back) {
            //       |--------seg-------|
            // |-----range-----|
            return splitSegment(manager, begin, range.back, kLow);
        } else if (range.front > seg.front && range.back > seg.back) {
            // |--------seg-------|
            //       |-----range-----|
            return splitSegment(manager, begin, range.front, kHigh);
        } else {
            MemLog.fatalf("Invalid state: ", range, ", ", seg);
            KM_ASSERT(false);
        }
    } else if (end != segments().end()) {
        const auto& lhs = begin->second;
        const auto& rhs = end->second;
        auto lhsMapping = lhs.mapping();
        auto rhsMapping = rhs.mapping();
        auto lhsVirtualRange = lhsMapping.virtualRange();
        auto rhsVirtualRange = rhsMapping.virtualRange();
        if (lhsVirtualRange.front == range.front && rhsVirtualRange.back == range.back) {
            // |----lhs----|   |----rhs----|
            // |-----------range-----------|

            eraseSegment(manager, lhsVirtualRange);
            eraseSegment(manager, rhsVirtualRange);

            eraseMany(manager, { lhsVirtualRange.back, rhsVirtualRange.front });
            return OsStatusSuccess;
        } else if (range.contains(lhsVirtualRange) && rhsVirtualRange.back == range.back) {
            //       |----lhs----|   |----rhs----|
            //   |--------------range------------|

            eraseSegment(manager, lhsVirtualRange);
            eraseSegment(manager, rhsVirtualRange);

            km::VirtualRange inner { lhsVirtualRange.back, rhsVirtualRange.front };
            eraseMany(manager, inner);

            return OsStatusSuccess;
        } else if (range.contains(rhsVirtualRange) && lhsVirtualRange.front == range.front) {
            // |----lhs----|   |----rhs----|
            // |--------------range------------|

            eraseSegment(manager, lhsVirtualRange);
            eraseSegment(manager, rhsVirtualRange);

            km::VirtualRange inner { lhsVirtualRange.back, rhsVirtualRange.front };
            eraseMany(manager, inner);

            return OsStatusSuccess;
        } else if (lhsVirtualRange.contains(range.front) && rhsVirtualRange.contains(range.back)) {
            // |----lhs----|   |----rhs----|
            //     |-------range-------|
            splitSegment(manager, mTable.find(range.front), range.front, kHigh);
            splitSegment(manager, mTable.find(range.back), range.back, kLow);

            eraseMany(manager, { lhsVirtualRange.back, rhsVirtualRange.front });
            return OsStatusSuccess;
        } else if (range.front > lhsVirtualRange.front && range.back > rhsVirtualRange.back) {
            // |----lhs----|   |----rhs----|
            //     |-----------range----------|
            splitSegment(manager, begin, range.front, kHigh);
            eraseSegment(manager, rhsVirtualRange);

            km::VirtualRange inner { lhsVirtualRange.back, rhsVirtualRange.front };
            eraseMany(manager, inner);

            return OsStatusSuccess;
        } else if (lhsVirtualRange.contains(range.front)) {
            // |--------seg-------|
            //     |--------range-----|
            MemLog.fatalf("Invalid range state: ", range, ", ", lhsMapping, ", ", rhsMapping);
            KM_ASSERT(false);
        } else {
            MemLog.fatalf("Invalid state: ", range, ", ", lhsMapping, ", ", rhsMapping);
            KM_ASSERT(false);
        }
    }

    end = mTable.find(range.back);

    km::VirtualRange remaining = range;
    for (auto it = begin; it != end;) {
        switch (OsStatus status = unmapSegment(manager, it, range, &remaining)) {
        case OsStatusCompleted:
            return OsStatusSuccess;
        case OsStatusSuccess:
            std::tie(it, end) = mTable.find(remaining);
            continue;
        default:
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::querySegment(const void *address, AddressSegment *result) noexcept [[clang::nonallocating]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    stdx::LockGuard guard(mLock);

    auto it = segments().upper_bound(address);
    if (it == segments().end()) {
        return OsStatusNotFound;
    }

    auto& segment = it->second;
    if (segment.range().contains(address)) {
        *result = segment;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;

    CLANG_DIAGNOSTIC_POP();
}

sys::AddressSpaceManagerStats AddressSpaceManager::stats() noexcept [[clang::nonallocating]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    stdx::LockGuard guard(mLock);

    return sys::AddressSpaceManagerStats {
        .heapStats = mHeap.stats(),
        .segments = segments().size(),
    };

    CLANG_DIAGNOSTIC_POP();
}

OsStatus AddressSpaceManager::create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager [[outparam]]) [[clang::allocating]] {
    km::VmemHeap heap;
    km::PageTables pt;

    if (OsStatus status = km::PageTables::create(pm, pteMemory, flags, &pt)) {
        return status;
    }

    if (OsStatus status = km::VmemHeap::create(vmem.cast<sm::VirtualAddress>(), &heap)) {
        return status;
    }

    *manager = sys::AddressSpaceManager(pteMemory, std::move(pt), std::move(heap));
    return OsStatusSuccess;
}

OsStatus AddressSpaceManager::create(const km::AddressSpace *pt, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager [[outparam]]) [[clang::allocating]] {
    if (OsStatus status = AddressSpaceManager::create(pt->pageManager(), pteMemory, flags, vmem, manager)) {
        return status;
    }

    km::copyHigherHalfMappings(&manager->mPageTables, pt->tables());
    return OsStatusSuccess;
}

void AddressSpaceManager::setActiveMap(const AddressSpaceManager *map) noexcept {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

    const km::PageBuilder *pm = map->mPageTables.pageManager();
    pm->setActiveMap(map->mPageTables.root());

    CLANG_DIAGNOSTIC_POP();
}

km::PhysicalAddressEx AddressSpaceManager::getPageMap() const noexcept [[clang::nonallocating]] {
    return mPageTables.root();
}

void AddressSpaceManager::dump() noexcept {
    stdx::LockGuard guard(mLock);
    for (const auto& [_, segment] : segments()) {
        MemLog.dbgf("Segment: ", segment.getBackingMemory(), " ", segment.range());
    }
}

void AddressSpaceManager::destroy(MemoryManager *manager) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    auto it = segments().begin();
    while (it != segments().end()) {
        it = eraseSegment(manager, it);
    }
}
