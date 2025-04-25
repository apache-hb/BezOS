#include "system/pmm.hpp"
#include "arch/paging.hpp"

OsStatus sys2::MemoryManager::retain(km::MemoryRange range) [[clang::allocating]] {
    auto begin = mSegments.lower_bound(range.front.address);
    auto end = mSegments.upper_bound(range.back.address);

    if (begin == end) {
        return OsStatusInvalidInput;
    }

    if (begin != mSegments.begin()) {
        --begin;
    }

    for (auto it = begin; it != end; ++it) {
        auto& segment = it->second;
        if (range.contains(segment.mHandle.range())) {
            segment.mOwners += 1;
        } else {
            auto intersection = km::intersection(range, segment.mHandle.range());
            km::TlsfAllocation lo, hi;
            if (OsStatus status = mHeap.split(segment.mHandle, intersection.front, &lo, &hi)) {
                return status;
            }
            segment.mHandle = lo;

            auto newSegment = MemorySegment { uint8_t(segment.mOwners + 1), hi };
            mSegments.insert({ hi.address(), std::move(newSegment) });

            segment.mOwners += 1;

            begin = mSegments.lower_bound(lo.address());
            end = mSegments.upper_bound(range.back);
        }
    }

    return OsStatusSuccess;
}

bool sys2::MemoryManager::releaseEntry(Iterator it) {
    auto& segment = it->second;
    if (releaseSegment(segment)) {
        mSegments.erase(it);
        return true;
    }

    return false;
}

bool sys2::MemoryManager::releaseSegment(sys2::MemorySegment& segment) {
    if (segment.mOwners.fetch_sub(1) == 1) {
        mHeap.free(segment.mHandle);
        return true;
    }
    return false;
}

void sys2::MemoryManager::addSegment(MemorySegment&& segment) {
    auto range = segment.mHandle.range();
    mSegments.insert({ range.back, std::move(segment) });
}

OsStatus sys2::MemoryManager::splitSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) [[clang::allocating]] {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.mHandle, midpoint, &lo, &hi)) {
        return status;
    }

    auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
    auto hiSegment = MemorySegment { uint8_t(segment.mOwners), hi };

    mSegments.erase(it);

    switch (side) {
    case kLow:
        if (!releaseSegment(loSegment)) {
            addSegment(std::move(loSegment));
        }
        addSegment(std::move(hiSegment));
        break;
    case kHigh:
        if (!releaseSegment(hiSegment)) {
            addSegment(std::move(hiSegment));
        }
        addSegment(std::move(loSegment));
        break;
    default:
        KM_PANIC("Invalid side");
        break;
    }

    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::releaseRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) {
    auto& segment = it->second;
    km::MemoryRange seg = segment.mHandle.range();
    km::TlsfAllocation allocation = segment.mHandle;
    if (seg == range) {
        // |--------seg-------|
        // |--------range-----|
        releaseEntry(it);
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
        releaseEntry(it);
        *remaining = { seg.back, range.back };
        return OsStatusSuccess;
    } else if (km::innerAdjacent(seg, range)) {
        // handle the following cases:
        //
        // |--------seg-------|
        // |--range--|
        //
        // |--------seg-------|
        //          |--range--|
        if (range.front == seg.front) {
            km::TlsfAllocation lo, hi;
            if (OsStatus status = mHeap.split(allocation, range.back, &lo, &hi)) {
                return status;
            }

            auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
            auto hiSegment = MemorySegment { uint8_t(segment.mOwners), hi };

            mSegments.erase(it);

            if (!releaseSegment(loSegment)) {
                addSegment(std::move(loSegment));
            }

            addSegment(std::move(hiSegment));

            return OsStatusCompleted;
        } else {
            KM_ASSERT(range.back == seg.back);

            km::TlsfAllocation lo, hi;
            if (OsStatus status = mHeap.split(allocation, range.front, &lo, &hi)) {
                return status;
            }

            auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
            auto hiSegment = MemorySegment { uint8_t(segment.mOwners), hi };

            mSegments.erase(it);

            if (!releaseSegment(hiSegment)) {
                addSegment(std::move(hiSegment));
            }

            addSegment(std::move(loSegment));

            return OsStatusCompleted;
        }
    } else if (seg.contains(range)) {
        // |--------seg-------|
        //       |--range--|
        auto [lhs, rhs] = km::split(seg, range);
        KM_ASSERT(!lhs.isEmpty() && !rhs.isEmpty());

        km::TlsfAllocation lo, mid, hi;
        if (OsStatus status = mHeap.split(allocation, lhs.back, &lo, &mid)) {
            return status;
        }

        if (OsStatus status = mHeap.split(mid, rhs.front, &mid, &hi)) {
            return status;
        }

        auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
        auto midSegment = MemorySegment { uint8_t(segment.mOwners), mid };
        auto hiSegment = MemorySegment { uint8_t(segment.mOwners), hi };

        mSegments.erase(it);

        if (!releaseSegment(midSegment)) {
            addSegment(std::move(midSegment));
        }

        addSegment(std::move(loSegment));
        addSegment(std::move(hiSegment));

        return OsStatusCompleted;
    } else {
        if (seg.front > range.front) {
            //       |--------seg-------|
            // |--------range-----|
            if (OsStatus status = splitSegment(it, range.back, kLow)) {
                return status;
            }

            return OsStatusCompleted;
        } else {
            // |--------seg-------|
            //       |--------range-----|
            if (OsStatus status = splitSegment(it, range.front, kHigh)) {
                return status;
            }

            *remaining = { seg.back, range.back };
            return OsStatusSuccess;
        }
    }

    KM_PANIC("Invalid state");
}

OsStatus sys2::MemoryManager::release(km::MemoryRange range) [[clang::allocating]] {
    Iterator begin;
    Iterator end;

    auto updateIterators = [&](km::MemoryRange newRange) {
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

    km::MemoryRange remaining = range;
    for (auto it = begin; it != end;) {
        switch (OsStatus status = releaseRange(it, remaining, &remaining)) {
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

OsStatus sys2::MemoryManager::allocate(size_t size, km::MemoryRange *range) [[clang::allocating]] {
    km::TlsfAllocation allocation = mHeap.aligned_alloc(alignof(x64::page), size);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    auto result = allocation.range();
    auto segment = MemorySegment { allocation };

    addSegment(std::move(segment));

    *range = result;
    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]] {
    km::TlsfHeap heap;
    if (OsStatus status = km::TlsfHeap::create(range, &heap)) {
        return status;
    }

    *manager = MemoryManager(range, std::move(heap));
    return OsStatusSuccess;
}

sys2::MemoryManagerStats sys2::MemoryManager::stats() noexcept [[clang::nonallocating]] {
    return MemoryManagerStats {
        .heapStats = mHeap.stats(),
        .segments = mSegments.size(),
    };
}

OsStatus sys2::MemoryManager::querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]] {
    auto it = mSegments.lower_bound(address);
    if (it == mSegments.end()) {
        return OsStatusNotFound;
    }
    auto& segment = it->second;
    auto span = segment.mHandle.range();
    bool found = span.contains(address) || span.front == address || span.back == address;
    if (!found) {
        return OsStatusNotFound;
    }

    *stats = segment.stats();
    return OsStatusSuccess;
}
