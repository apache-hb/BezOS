#include "system/vmm.hpp"
#include "memory/address_space.hpp"
#include "memory/tables.hpp"
#include "system/pmm.hpp"

#include "common/util/defer.hpp"

OsStatus sys::AddressSpaceManager::map(MemoryManager *manager, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::TlsfAllocation allocation = mHeap.aligned_alloc(align, size);
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

    auto segment = AddressSegment { range, allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus sys::AddressSpaceManager::mapSegment(
    MemoryManager *manager, Iterator it,
    km::VirtualRange src, km::VirtualRange dst,
    km::PageFlags flags, km::MemoryType type,
    km::TlsfAllocation allocation
) [[clang::allocating]] {
    OsStatus status = OsStatusSuccess;
    auto& segment = it->second;
    auto segmentMapping = segment.mapping();
    km::VirtualRange seg = segmentMapping.virtualRange();

    km::VirtualRange subrange = km::intersection(seg, src);
    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    km::MemoryRange memory = { segment.backing.front + frontOffset, segment.backing.back - backOffset };

    size_t totalFrontOffset = (uintptr_t)subrange.front - (uintptr_t)src.front;
    size_t totalBackOffset = (uintptr_t)src.back - (uintptr_t)subrange.back;
    km::VirtualRange dstRange = { (void*)((uintptr_t)dst.front + totalFrontOffset), (void*)((uintptr_t)dst.back - totalBackOffset) };

    km::AddressMapping mapping {
        .vaddr = dstRange.front,
        .paddr = memory.front,
        .size = memory.size(),
    };

    status = manager->retain(memory);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[VMM] Failed to retain memory: ", memory, " ", seg, " ", src, " ", OsStatusId(status), "\n");
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

        km::TlsfAllocation lo, mid, hi;
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

OsStatus sys::AddressSpaceManager::map(MemoryManager *manager, AddressSpaceManager *other, km::VirtualRange range, km::PageFlags flags, km::MemoryType type, km::VirtualRange *result) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    stdx::LockGuard guard2(other->mLock); // TODO: this can deadlock

    auto [begin, end] = other->mTable.find(range);

    if (begin == other->mTable.end()) {
        return OsStatusNotFound;
    }

    km::TlsfAllocation allocation = mHeap.aligned_alloc(alignof(x64::page), range.size());
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    if (begin == end) {
        auto segment = begin->second;
        auto seg = segment.range();
        if (seg == range) {
            OsStatus status = OsStatusSuccess;
            km::MemoryRange memory = segment.backing;
            km::AddressMapping mapping {
                .vaddr = std::bit_cast<const void*>(allocation.address()),
                .paddr = memory.front,
                .size = memory.size(),
            };

            status = manager->retain(memory);
            if (status != OsStatusSuccess) {
                KmDebugMessage("[VMM] Failed to retain memory: ", memory, " ", seg, " ", range, " ", OsStatusId(status), "\n");
                KM_ASSERT(status == OsStatusSuccess);
            }

            status = mPageTables.map(mapping, flags, type);
            if (status != OsStatusSuccess) {
                KmDebugMessage("[VMM] Failed to map memory: ", mapping, " ", seg, " ", range, " ", OsStatusId(status), "\n");
                KM_ASSERT(status == OsStatusSuccess);
            }

            AddressSegment newSegment { memory, allocation };
            addSegment(std::move(newSegment));
            *result = mapping.virtualRange();
            return OsStatusSuccess;
        } else {
            KmDebugMessage("Unsupported mapping: ", seg, " ", range, "\n");
            KM_ASSERT(false);
        }
    } else if (end != mTable.end()) {
        KmDebugMessage("Unsupported mapping: ", begin->second.range(), " ", end->second.range(), " ", range, "\n");
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

void sys::AddressSpaceManager::addSegment(AddressSegment &&segment) noexcept [[clang::allocating]] {
    mTable.insert(std::move(segment));
}

void sys::AddressSpaceManager::addNoAccessSegment(km::TlsfAllocation allocation) noexcept [[clang::allocating]] {
    auto segment = AddressSegment { km::MemoryRange{}, allocation };
    addSegment(std::move(segment));
}

OsStatus sys::AddressSpaceManager::splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]] {
    auto& segment = it->second;

    km::MemoryRange memory = segment.backing;
    auto seg = segment.range();
    size_t frontOffset = (uintptr_t)midpoint - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)midpoint;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.allocation, std::bit_cast<uintptr_t>(midpoint), &lo, &hi)) {
        return status;
    }

    km::MemoryRange loRange = { memory.front, memory.front + frontOffset };
    km::MemoryRange hiRange = { memory.back - backOffset, memory.back };

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

OsStatus sys::AddressSpaceManager::unmapSegment(MemoryManager *manager, Iterator it, km::VirtualRange range, km::VirtualRange *remaining) [[clang::allocating]] {
    auto& segment = it->second;
    OsStatus status = OsStatusSuccess;
    km::MemoryRange memory = segment.backing;
    km::VirtualRange seg = segment.range();

    km::VirtualRange subrange = km::intersection(seg, range);
    if (subrange.isEmpty()) {
        return OsStatusCompleted;
    }

    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    km::MemoryRange srcMemory = { segment.backing.front + frontOffset, segment.backing.back - backOffset };
    if (range.contains(seg)) {
        // |-----seg-----|
        // |--------range-----|
        //
        //      |-----seg-----|
        // |--------range-----|
        //
        //    |-----seg-----|
        // |--------range-----|
        status = manager->release(srcMemory);
        KM_ASSERT(status == OsStatusSuccess);

        status = mPageTables.unmap(seg);
        KM_ASSERT(status == OsStatusSuccess);

        mHeap.free(segment.allocation);

        mTable.erase(it);
        *remaining = { seg.back, range.back };
        return OsStatusSuccess;
    } else if (seg.contains(range) && range.front != seg.front) {
        // |--------seg-------|
        //       |--range--|
        KmDebugMessage("Splitting segment: ", seg, ", ", range, "\n");
        KM_PANIC("Unsupported mapping");
        auto [lhs, rhs] = km::split(seg, range);
        KM_ASSERT(!lhs.isEmpty() && !rhs.isEmpty());

        km::TlsfAllocation lo, mid, hi;
        if (OsStatus status = mHeap.split(segment.allocation, std::bit_cast<uintptr_t>(lhs.back), &lo, &mid)) {
            return status;
        }

        if (OsStatus status = mHeap.split(mid, std::bit_cast<uintptr_t>(rhs.front), &mid, &hi)) {
            return status;
        }

        km::MemoryRange loRange = { memory.front, memory.back + lhs.size() };
        km::MemoryRange hiRange = { memory.back - rhs.size(), memory.back };
        auto loSegment = AddressSegment { loRange, lo };
        auto hiSegment = AddressSegment { hiRange, hi };
        km::MemoryRange midRange = { memory.front + lhs.size(), memory.back - rhs.size() };

        mTable.erase(it);

        OsStatus status = manager->release(midRange);
        KM_ASSERT(status == OsStatusSuccess);

        addSegment(std::move(loSegment));
        addSegment(std::move(hiSegment));

        return OsStatusCompleted;
    } else if (km::innerAdjacent(seg, range)) {
        KmDebugMessage("Splitting segment: ", seg, ", ", range, "\n");
        KM_PANIC("Unsupported mapping");
        if (range.front == seg.front) {
            // |--------seg-------|
            // |--range--|
            if (OsStatus status = splitSegment(manager, it, range.back, kLow)) {
                return status;
            }
        } else {
            KM_ASSERT(range.back == seg.back);
            // |--------seg-------|
            //          |--range--|
            if (OsStatus status = splitSegment(manager, it, range.front, kHigh)) {
                return status;
            }
        }

        return OsStatusCompleted;
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

OsStatus sys::AddressSpaceManager::map(MemoryManager *manager, sm::VirtualAddress address, km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]] {
    // TODO: on the error case memory is a bit busted
    km::VirtualRange range = { address, address + memory.size() };
    if (OsStatus status = unmap(manager, range)) {
        if (status != OsStatusNotFound) {
            return status;
        }
    }

    km::TlsfAllocation allocation;

    stdx::LockGuard guard(mLock);
    if (OsStatus status = mHeap.reserve(range.cast<km::PhysicalAddress>(), &allocation)) {
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

    auto segment = AddressSegment { memory, allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

OsStatus sys::AddressSpaceManager::mapExternal(km::MemoryRange memory, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::TlsfAllocation allocation = mHeap.aligned_alloc(alignof(x64::page), memory.size());

    km::AddressMapping result {
        .vaddr = std::bit_cast<const void*>(allocation.address()),
        .paddr = memory.front,
        .size = memory.size(),
    };

    if (OsStatus status = mPageTables.map(result, flags, type)) {
        KM_ASSERT(status == OsStatusSuccess);
        return status;
    }

    auto segment = AddressSegment { km::MemoryRange{}, allocation };
    addSegment(std::move(segment));

    *mapping = result;
    return OsStatusSuccess;
}

void sys::AddressSpaceManager::deleteSegment(MemoryManager *manager, AddressSegment&& segment) noexcept [[clang::allocating]] {
    OsStatus status = OsStatusSuccess;

    if (segment.hasBackingMemory()) {
        status = manager->release(segment.backing);
        KM_ASSERT(status == OsStatusSuccess);
    }

    status = mPageTables.unmap(segment.range());
    KM_ASSERT(status == OsStatusSuccess);

    mHeap.free(segment.allocation);
}

sys::AddressSpaceManager::Iterator sys::AddressSpaceManager::eraseSegment(MemoryManager *manager, Iterator it) noexcept [[clang::allocating]] {
    auto& segment = it->second;

    deleteSegment(manager, std::move(segment));

    return mTable.erase(it);
}

void sys::AddressSpaceManager::eraseSegment(MemoryManager *manager, km::VirtualRange segment) noexcept [[clang::allocating]] {
    auto it = segments().find(segment.back);
    KM_ASSERT(it != segments().end());
    eraseSegment(manager, it);
}

void sys::AddressSpaceManager::eraseMany(MemoryManager *manager, km::VirtualRange range) noexcept [[clang::allocating]] {
    auto begin = segments().upper_bound(range.front);
    auto end = segments().upper_bound(range.back);

    for (auto it = begin; it != end;) {
        auto& segment = it->second;
        KM_ASSERT(range.contains(segment.range()));
        it = eraseSegment(manager, it);
        end = segments().upper_bound(range.back);
    }
}

OsStatus sys::AddressSpaceManager::unmap(MemoryManager *manager, km::VirtualRange range) [[clang::allocating]] {
    KM_ASSERT(range.isValid());

    stdx::LockGuard guard(mLock);

    defer {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety-analysis"); // The lock is held, but the compiler doesn't know it

        auto upper = segments().upper_bound(range.front);
        if (upper != segments().end()) {
            auto inner = upper->second.range();
            auto intersect = km::intersection(range, inner);
            if (!intersect.isEmpty()) {
                KmDebugMessage("Failed to unmap: ", range, ", ", inner, ", ", intersect, "\n");
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

            std::array<km::TlsfAllocation, 3> allocations;
            std::array<km::PhysicalAddress, 2> points = {
                (uintptr_t)range.front,
                (uintptr_t)range.back,
            };

            if (OsStatus status = mHeap.splitv(segment.allocation, points, allocations)) {
                return status;
            }

            status = manager->release(subrange.physicalRange());
            KM_ASSERT(status == OsStatusSuccess);

            status = mPageTables.unmap(subrange.virtualRange());
            KM_ASSERT(status == OsStatusSuccess);

            AddressSegment loSegment { segment.backing.first(lhs.size()), allocations[0] };
            AddressSegment hiSegment { segment.backing.last(rhs.size()), allocations[2] };

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
            KmDebugMessage("Invalid state: ", range, ", ", seg, "\n");
            KM_ASSERT(false);
        }
    } else if (end != segments().end()) {
        auto lhs = begin->second;
        auto rhs = end->second;
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
            KmDebugMessage("Invalid range state: ", range, ", ", lhsMapping, ", ", rhsMapping, "\n");
            KM_ASSERT(false);
        } else {
            KmDebugMessage("Invalid state: ", range, ", ", lhsMapping, ", ", rhsMapping, "\n");
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

OsStatus sys::AddressSpaceManager::querySegment(const void *address, AddressSegment *result) noexcept [[clang::nonallocating]] {
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

sys::AddressSpaceManagerStats sys::AddressSpaceManager::stats() noexcept [[clang::nonallocating]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    stdx::LockGuard guard(mLock);

    return sys::AddressSpaceManagerStats {
        .heapStats = mHeap.stats(),
        .segments = segments().size(),
    };

    CLANG_DIAGNOSTIC_POP();
}

OsStatus sys::AddressSpaceManager::create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]] {
    km::TlsfHeap heap;
    km::MemoryRange range { (uintptr_t)vmem.front, (uintptr_t)vmem.back };
    if (OsStatus status = km::TlsfHeap::create(range, &heap)) {
        return status;
    }

    *manager = sys::AddressSpaceManager(pm, pteMemory, flags, std::move(heap));
    return OsStatusSuccess;
}

OsStatus sys::AddressSpaceManager::create(const km::AddressSpace *pt, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]] {
    if (OsStatus status = AddressSpaceManager::create(pt->pageManager(), pteMemory, flags, vmem, manager)) {
        return status;
    }

    km::copyHigherHalfMappings(&manager->mPageTables, pt->tables());
    return OsStatusSuccess;
}

void sys::AddressSpaceManager::setActiveMap(const AddressSpaceManager *map) noexcept {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

    const km::PageBuilder *pm = map->mPageTables.pageManager();
    pm->setActiveMap(map->mPageTables.root());

    CLANG_DIAGNOSTIC_POP();
}

void sys::AddressSpaceManager::destroy(MemoryManager *manager) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    auto it = segments().begin();
    while (it != segments().end()) {
        it = eraseSegment(manager, it);
    }
}
