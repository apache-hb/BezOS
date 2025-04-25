#include "system/vmm.hpp"
#include "system/pmm.hpp"

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

void sys2::AddressSpaceManager::addSegment(AddressSegment &&segment) noexcept [[clang::allocating]] {
    auto range = segment.virtualRange();
    mSegments.insert({ range.back, segment });
}

OsStatus sys2::AddressSpaceManager::splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]] {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.allocation, std::bit_cast<uintptr_t>(midpoint), &lo, &hi)) {
        return status;
    }

    auto loSegment = AddressSegment { segment.range, lo };
    auto hiSegment = AddressSegment { segment.range, hi };

    mSegments.erase(it);

    switch (side) {
    case kLow:
        addSegment(std::move(hiSegment));
        {
            OsStatus status = manager->release(loSegment.range);
            KM_ASSERT(status == OsStatusSuccess);
        }
        break;
    case kHigh:
        addSegment(std::move(loSegment));
        {
            OsStatus status = manager->release(hiSegment.range);
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
    if (seg == range) {
        // |--------seg-------|
        // |--------range-----|
        status = manager->release(memory);
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
        status = manager->release(memory);
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

            return OsStatusCompleted;
        } else {
            KM_ASSERT(range.back == seg.back);
            // |--------seg-------|
            //          |--range--|

            if (OsStatus status = splitSegment(manager, it, range.front, kHigh)) {
                return status;
            }

            return OsStatusCompleted;
        }
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
    Iterator begin;
    Iterator end;

    auto updateIterators = [&](km::VirtualRange newRange) {
        begin = mSegments.lower_bound(newRange.front);
        end = mSegments.lower_bound(newRange.back);

        if (end != mSegments.end()) {
            ++end;
        }
    };

    updateIterators(range);
    if (begin == mSegments.end()) {
        return OsStatusNotFound;
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

OsStatus sys2::AddressSpaceManager::create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]] {
    km::TlsfHeap heap;
    km::MemoryRange range { (uintptr_t)vmem.front, (uintptr_t)vmem.back };
    if (OsStatus status = km::TlsfHeap::create(range, &heap)) {
        return status;
    }

    *manager = sys2::AddressSpaceManager(pm, pteMemory, flags, std::move(heap));
    return OsStatusSuccess;
}
