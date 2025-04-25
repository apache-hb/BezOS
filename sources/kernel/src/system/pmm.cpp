#include "system/pmm.hpp"
#include "arch/paging.hpp"
#include "log.hpp"

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

bool sys2::MemoryManager::releaseSegment(Iterator it) {
    auto& segment = it->second;
    segment.mOwners -= 1;
    if (segment.mOwners == 0) {
        mHeap.free(segment.mHandle);
        mSegments.erase(it);
        return true;
    }

    return false;
}

OsStatus sys2::MemoryManager::release(km::MemoryRange range) [[clang::allocating]] {
    auto begin = mSegments.lower_bound(range.front.address);
    auto end = mSegments.upper_bound(range.back.address);

    if (begin != mSegments.begin()) {
        --begin;
    }

    if (begin == mSegments.end()) {
        return OsStatusInvalidInput;
    }

    for (auto it = begin; it != end; ++it) {
        auto& segment = it->second;
        auto seg = segment.mHandle.range();
        if (seg == range) {
            releaseSegment(it);
            return OsStatusSuccess;
        } else if (seg.contains(range)) {
            auto [lhs, rhs] = km::split(seg, range);

            if (!lhs.isEmpty() && !rhs.isEmpty()) {
                km::TlsfAllocation lo, mid, hi;
                if (OsStatus status = mHeap.split(segment.mHandle, lhs.back, &lo, &mid)) {
                    KmDebugMessage("Failed to split segment ", range, " ", seg, " ", lhs, " ", rhs);
                    return status;
                }

                if (OsStatus status = mHeap.split(mid, rhs.front, &mid, &hi)) {
                    KmDebugMessage("Failed to split segment high ", range, " ", seg, " ", lhs, " ", rhs);
                    return status;
                }

                auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
                auto midSegment = MemorySegment { uint8_t(segment.mOwners - 1), mid };
                auto hiSegment = MemorySegment { uint8_t(segment.mOwners), hi };

                mSegments.erase(begin);

                if (midSegment.mOwners == 0) {
                    mHeap.free(midSegment.mHandle);
                } else {
                    mSegments.insert({ mid.address(), std::move(midSegment) });
                }
                mSegments.insert({ lo.address(), std::move(loSegment) });
                mSegments.insert({ hi.address(), std::move(hiSegment) });
            } else if (lhs.isEmpty()) {
                km::TlsfAllocation lo, hi;
                if (OsStatus status = mHeap.split(segment.mHandle, rhs.front, &lo, &hi)) {
                    KmDebugMessage("Failed to split segment ", range, " ", seg, " ", lhs, " ", rhs);
                    return status;
                }

                auto loSegment = MemorySegment { uint8_t(segment.mOwners - 1), hi };
                auto hiSegment = MemorySegment { uint8_t(segment.mOwners), lo };
                mSegments.erase(it);

                if (loSegment.mOwners == 0) {
                    mHeap.free(loSegment.mHandle);
                } else {
                    mSegments.insert({ lo.address(), std::move(loSegment) });
                }
                mSegments.insert({ hi.address(), std::move(hiSegment) });
            } else if (rhs.isEmpty()) {
                km::TlsfAllocation lo, hi;
                if (OsStatus status = mHeap.split(segment.mHandle, lhs.back, &lo, &hi)) {
                    KmDebugMessage("Failed to split segment ", range, " ", seg, " ", lhs, " ", rhs);
                    return status;
                }

                auto loSegment = MemorySegment { uint8_t(segment.mOwners), lo };
                auto hiSegment = MemorySegment { uint8_t(segment.mOwners - 1), hi };
                mSegments.erase(it);

                if (hiSegment.mOwners == 0) {
                    mHeap.free(hiSegment.mHandle);
                } else {
                    mSegments.insert({ hi.address(), std::move(hiSegment) });
                }
                mSegments.insert({ lo.address(), std::move(loSegment) });
            } else {
                return OsStatusInvalidInput;
            }

            return OsStatusSuccess;
        } else if (range.contains(seg)) {
            if (releaseSegment(it)) {
                begin = mSegments.lower_bound(range.front);
                end = mSegments.upper_bound(range.back.address);
            }
        } else if (km::innerAdjacent(range, seg)) {
            auto subrange = range.cut(seg);
            KM_CHECK(!subrange.isEmpty(), "Subrange is empty");

            km::TlsfAllocation lo, hi;
            if (OsStatus status = mHeap.split(segment.mHandle, subrange.front, &lo, &hi)) {
                KmDebugMessage("Failed to split segment ", range, " ", seg, " ", subrange);
                return status;
            }

            if (range.front == seg.front) {
                auto newSegment = MemorySegment { uint8_t(segment.mOwners - 1), lo };
                mHeap.free(hi);

                mSegments.erase(it);
                mSegments.insert({ lo.address(), std::move(newSegment) });
            } else {
                auto newSegment = MemorySegment { uint8_t(segment.mOwners), hi };
                mHeap.free(lo);

                mSegments.erase(it);
                mSegments.insert({ hi.address(), std::move(newSegment) });
            }
        } else {
            auto intersection = km::intersection(range, seg);
            KM_CHECK(!intersection.isEmpty(), "Intersection is empty");

            km::TlsfAllocation lo, hi;
            if (OsStatus status = mHeap.split(segment.mHandle, intersection.front, &lo, &hi)) {
                KmDebugMessage("Failed to split segment ", range, " ", seg, " ", intersection);
                return status;
            }
            segment.mHandle = lo;

            auto newSegment = MemorySegment { uint8_t(segment.mOwners - 1), hi };
            mSegments.insert({ hi.address(), std::move(newSegment) });

            segment.mOwners -= 1;

            begin = mSegments.lower_bound(lo.address());
            end = mSegments.upper_bound(range.back);
        }
    }

    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::allocate(size_t size, km::MemoryRange *range) [[clang::allocating]] {
    km::TlsfAllocation allocation = mHeap.aligned_alloc(alignof(x64::page), size);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    auto address = allocation.address();
    auto result = allocation.range();
    auto segment = MemorySegment { allocation };

    mSegments.insert({ address, std::move(segment) });

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
