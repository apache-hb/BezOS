#include "system/vmm.hpp"
#include "system/pmm.hpp"

#include "common/compiler/compiler.hpp"

OsStatus sys2::AddressSpaceManager::map(MemoryManager *manager, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]] {
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

OsStatus sys2::AddressSpaceManager::mapSegment(
    MemoryManager *manager, Iterator it,
    km::VirtualRange src, km::VirtualRange dst,
    km::PageFlags flags, km::MemoryType type,
    km::TlsfAllocation allocation,
    km::TlsfAllocation *remaining
) [[clang::allocating]] {
    OsStatus status = OsStatusSuccess;
    auto& segment = it->second;
    auto segmentMapping = segment.mapping();
    km::VirtualRange seg = segmentMapping.virtualRange();

    km::VirtualRange subrange = km::intersection(seg, src);
    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    km::MemoryRange memory = { segment.range.front + frontOffset, segment.range.back - backOffset };

    size_t totalFrontOffset = (uintptr_t)subrange.front - (uintptr_t)src.front;
    size_t totalBackOffset = (uintptr_t)src.back - (uintptr_t)subrange.back;
    km::VirtualRange dstRange = { (void*)((uintptr_t)dst.front + totalFrontOffset), (void*)((uintptr_t)dst.back - totalBackOffset) };

    km::AddressMapping mapping {
        .vaddr = dstRange.front,
        .paddr = memory.front,
        .size = memory.size(),
    };

    status = manager->retain(memory);
    KM_ASSERT(status == OsStatusSuccess);

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

OsStatus sys2::AddressSpaceManager::map(MemoryManager *manager, AddressSpaceManager *other, km::VirtualRange range, km::PageFlags flags, km::MemoryType type, km::VirtualRange *mapping) [[clang::allocating]] {
    Iterator begin = other->mSegments.lower_bound(range.front);
    Iterator end = other->mSegments.lower_bound(range.back);

    if (end != mSegments.end()) {
        ++end;
    }

    if (begin == end) {
        return OsStatusNotFound;
    }

    km::TlsfAllocation allocation = mHeap.aligned_alloc(alignof(x64::page), range.size());
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    km::VirtualRange dst = allocation.range().cast<const void*>();

    for (auto it = begin; it != end; ++it) {
        switch (OsStatus status = mapSegment(manager, it, range, dst, flags, type, allocation, &allocation)) {
        case OsStatusCompleted:
            *mapping = dst;
            return OsStatusSuccess;
        case OsStatusSuccess:
            continue;
        default:
            mHeap.free(allocation);
            return status;
        }
    }

    *mapping = dst;
    return OsStatusSuccess;
}

void sys2::AddressSpaceManager::addSegment(AddressSegment &&segment) noexcept [[clang::allocating]] {
    auto range = segment.virtualRange();
    mSegments.insert({ range.back, segment });
}

void sys2::AddressSpaceManager::addNoAccessSegment(km::TlsfAllocation allocation) noexcept [[clang::allocating]] {
    auto segment = AddressSegment { km::MemoryRange{}, allocation };
    addSegment(std::move(segment));
}

OsStatus sys2::AddressSpaceManager::splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]] {
    auto& segment = it->second;

    km::MemoryRange memory = segment.range;
    auto seg = segment.virtualRange();
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

    mSegments.erase(it);

    switch (side) {
    case kLow:
        addSegment(std::move(hiSegment));
        {
            OsStatus status = manager->release(loRange);
            KM_ASSERT(status == OsStatusSuccess);
        }
        break;
    case kHigh:
        addSegment(std::move(loSegment));
        {
            OsStatus status = manager->release(hiRange);
            KM_ASSERT(status == OsStatusSuccess);
        }
        break;
    default:
        KM_PANIC("Invalid side");
        break;
    }

    return OsStatusSuccess;
}

OsStatus sys2::AddressSpaceManager::unmapSegment(MemoryManager *manager, Iterator it, km::VirtualRange range, km::VirtualRange *remaining) [[clang::allocating]] {
    auto& segment = it->second;
    OsStatus status = OsStatusSuccess;
    km::MemoryRange memory = segment.range;
    km::VirtualRange seg = segment.virtualRange();

    km::VirtualRange subrange = km::intersection(seg, range);
    size_t frontOffset = (uintptr_t)subrange.front - (uintptr_t)seg.front;
    size_t backOffset = (uintptr_t)seg.back - (uintptr_t)subrange.back;
    km::MemoryRange srcMemory = { segment.range.front + frontOffset, segment.range.back - backOffset };
    if (seg == range) {
        // |--------seg-------|
        // |--------range-----|
        status = manager->release(srcMemory);
        KM_ASSERT(status == OsStatusSuccess);

        status = mPageTables.unmap(seg);
        KM_ASSERT(status == OsStatusSuccess);

        mHeap.free(segment.allocation);

        mSegments.erase(it);
        return OsStatusCompleted;
    } else if (range.contains(seg)) {
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

        mSegments.erase(it);
        *remaining = { seg.back, range.back };
        return OsStatusSuccess;
    } else if (km::innerAdjacent(seg, range)) {
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
    } else if (seg.contains(range)) {
        // |--------seg-------|
        //       |--range--|
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

        mSegments.erase(it);

        OsStatus status = manager->release(midRange);
        KM_ASSERT(status == OsStatusSuccess);

        addSegment(std::move(loSegment));
        addSegment(std::move(hiSegment));

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
            // |--------seg-------|
            //          |--range--|
            if (OsStatus status = splitSegment(manager, it, range.front, kHigh)) {
                return status;
            }

            return OsStatusCompleted;
        }
    }
}

OsStatus sys2::AddressSpaceManager::unmap(MemoryManager *manager, km::VirtualRange range) [[clang::allocating]] {
    Iterator begin = mSegments.lower_bound(range.front);
    Iterator end =  mSegments.lower_bound(range.back);

    auto updateIterators = [&](km::VirtualRange newRange) {
        begin = mSegments.lower_bound(newRange.front);
        end = mSegments.lower_bound(newRange.back);

        if (end != mSegments.end()) {
            ++end;
        }
    };

    if (begin == mSegments.end()) {
        return OsStatusNotFound;
    }


    //     |--------seg-------|
    // |--------range-----|

    // |--------seg-------|
    //     |--------range-----|

    // |----lhs----|   |----rhs----|
    // |-----------range-----------|

    // |----lhs----|   |----rhs----|
    // |-------range------|

    // |----lhs----|   |----rhs----|
    //       |--------range--------|

    // |----lhs----|   |----rhs----|
    //     |-------range-------|

    if (begin == end) {
        auto& segment = begin->second;
        auto mapping = segment.mapping();
        auto seg = mapping.virtualRange();
        if (seg == range) {
            // |--------seg-------|
            // |--------range-----|
            OsStatus status = OsStatusSuccess;

            status = manager->release(mapping.physicalRange());
            KM_ASSERT(status == OsStatusSuccess);

            status = mPageTables.unmap(seg);
            KM_ASSERT(status == OsStatusSuccess);

            mHeap.free(segment.allocation);

            mSegments.erase(begin);
            return OsStatusSuccess;
        } else if (km::innerAdjacent(seg, range)) {
            if (range.front == seg.front) {
                // |--------seg-------|
                // |---range---|
                return splitSegment(manager, begin, range.back, kLow);
            } else {
                KM_ASSERT(range.back == seg.back);
                // |--------seg-------|
                //        |---range---|
                return splitSegment(manager, begin, range.front, kHigh);
            }
        } else if (seg.contains(range)) {
            // |--------seg-------|
            //     |---range---|
            OsStatus status = OsStatusSuccess;

            km::AddressMapping subrange = mapping.subrange(range);
            auto physicalRange = subrange.physicalRange();

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

            AddressSegment loSegment { physicalRange, allocations[0] };
            AddressSegment hiSegment { physicalRange, allocations[2] };

            mSegments.erase(begin);

            addSegment(std::move(loSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        }
    } else if (end != mSegments.end()) {

    }

    km::VirtualRange remaining = range;
    for (auto it = begin; it != end;) {
        switch (OsStatus status = unmapSegment(manager, it, remaining, &remaining)) {
        case OsStatusCompleted:
            return OsStatusSuccess;
        case OsStatusSuccess:
            updateIterators(remaining);
            it = begin;
            continue;
        default:
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus sys2::AddressSpaceManager::querySegment(const void *address, AddressSegment *result) noexcept [[clang::nonallocating]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    auto it = mSegments.upper_bound(address);
    if (it == mSegments.end()) {
        return OsStatusNotFound;
    }

    auto& segment = it->second;
    if (segment.virtualRange().contains(address)) {
        *result = segment;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;

    CLANG_DIAGNOSTIC_POP();
}

sys2::AddressSpaceManagerStats sys2::AddressSpaceManager::stats() noexcept [[clang::nonallocating]] {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

    return sys2::AddressSpaceManagerStats {
        .heapStats = mHeap.stats(),
        .segments = mSegments.size(),
    };

    CLANG_DIAGNOSTIC_POP();
}

OsStatus sys2::AddressSpaceManager::create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]] {
    km::TlsfHeap heap;
    km::MemoryRange range { (uintptr_t)vmem.front, (uintptr_t)vmem.back };
    if (OsStatus status = km::TlsfHeap::create(range, &heap)) {
        return status;
    }

    *manager = sys2::AddressSpaceManager(pm, pteMemory, flags, std::move(heap));
    return OsStatusSuccess;
}

OsStatus sys2::AddressSpaceManager::destroy(MemoryManager *manager) [[clang::allocating]] {
    for (const auto& [_, segment] : mSegments) {
        OsStatus status = manager->release(segment.range);
        KM_ASSERT(status == OsStatusSuccess);
    }

    mSegments.clear();
    return OsStatusSuccess;
}
