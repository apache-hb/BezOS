#include "system/pmm.hpp"

#include "log.hpp"
#include "memory/heap_command_list.hpp"

#include "common/compiler/compiler.hpp"
#include "common/util/defer.hpp"

OsStatus sys2::MemoryManager::retainRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) {
    auto& segment = it->second;
    km::MemoryRange seg = segment.handle.range();

    if (range.contains(seg)) {
        // |-----seg-----|
        // |--------range-----|
        //
        //      |-----seg-----|
        // |--------range-----|
        //
        //    |-----seg-----|
        // |--------range-----|
        segment.owners += 1;
        *remaining = { seg.back, range.back };
        return OsStatusSuccess;
    } else {
        if (seg.back == range.front) {
            return OsStatusMoreData;
        }

        if (seg.front > range.front) {
            //       |--------seg-------|
            // |--------range-----|
            if (OsStatus status = retainSegment(it, range.back, kLow)) {
                return status;
            }

            return OsStatusCompleted;
        } else {
            // |--------seg-------|
            //       |--------range-----|
            if (OsStatus status = retainSegment(it, range.front, kHigh)) {
                return status;
            }

            *remaining = { seg.back, range.back };
            return OsStatusSuccess;
        }
    }
}

void sys2::MemoryManager::retainEntry(Iterator it) {
    auto& segment = it->second;
    segment.owners += 1;
}

void sys2::MemoryManager::retainEntry(km::PhysicalAddress address) {
    auto it = mSegments.find(address);
    KM_ASSERT(it != mSegments.end());
    retainEntry(it);
}

bool sys2::MemoryManager::releaseEntry(km::PhysicalAddress address) {
    auto it = mSegments.find(address);
    KM_ASSERT(it != mSegments.end());
    return releaseEntry(it);
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
    if (segment.owners.fetch_sub(1) == 1) {
        mHeap.free(segment.handle);
        return true;
    }
    return false;
}

void sys2::MemoryManager::addSegment(MemorySegment&& segment) {
    auto range = segment.handle.range();
    mSegments.insert({ range.back, std::move(segment) });
}

OsStatus sys2::MemoryManager::retainSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.handle, midpoint, &lo, &hi)) {
        return status;
    }

    auto loSegment = MemorySegment { uint8_t(segment.owners), lo };
    auto hiSegment = MemorySegment { uint8_t(segment.owners), hi };

    mSegments.erase(it);

    switch (side) {
    case kLow:
        loSegment.owners += 1;
        break;
    case kHigh:
        hiSegment.owners += 1;
        break;
    default:
        KM_PANIC("Invalid side");
        break;
    }

    addSegment(std::move(loSegment));
    addSegment(std::move(hiSegment));

    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::splitSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap.split(segment.handle, midpoint, &lo, &hi)) {
        return status;
    }

    auto loSegment = MemorySegment { uint8_t(segment.owners), lo };
    auto hiSegment = MemorySegment { uint8_t(segment.owners), hi };

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
    km::MemoryRange seg = segment.handle.range();
    KM_ASSERT(km::interval(seg, range));

    if (range.contains(seg)) {
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
}

OsStatus sys2::MemoryManager::retain(km::MemoryRange range) [[clang::allocating]] {
    Iterator begin = mSegments.lower_bound(range.front);
    Iterator end = mSegments.lower_bound(range.back);

    auto updateIterators = [&](km::MemoryRange newRange) {
        begin = mSegments.lower_bound(newRange.front);
        end = mSegments.lower_bound(newRange.back);

        if (end != mSegments.end()) {
            ++end;
        }
    };

    if (begin == mSegments.end()) {
        return OsStatusNotFound;
    }

    if (begin == end) {
        MemorySegment& segment = begin->second;
        km::MemoryRange seg = segment.range();
        if (seg == range) {
            retainEntry(begin);
            return OsStatusSuccess;
        } else if (seg.contains(range) && !km::innerAdjacent(seg, range)) {
            // |--------seg-------|
            //       |--range--|
            std::array<km::TlsfAllocation, 3> allocations;
            std::array<km::PhysicalAddress, 2> points = {
                range.front,
                range.back,
            };

            if (OsStatus status = mHeap.splitv(segment.handle, points, allocations)) {
                return status;
            }

            MemorySegment loSegment { uint8_t(segment.owners), allocations[0] };
            MemorySegment midSegment { uint8_t(segment.owners + 1), allocations[1] };
            MemorySegment hiSegment { uint8_t(segment.owners), allocations[2] };

            mSegments.erase(end);

            addSegment(std::move(loSegment));
            addSegment(std::move(midSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        } else if (seg.contains(range.back) && seg.front == range.front) {
            // |--------seg-------|
            // |----range----|
            return retainSegment(begin, range.back, kLow);
        } else if (seg.contains(range.front) && seg.back == range.back) {
            // |--------seg-------|
            //     |----range-----|
            return retainSegment(begin, range.front, kHigh);
        } else {
            km::MemoryRange remaining = range;
            switch (OsStatus status = retainRange(begin, range, &remaining)) {
            case OsStatusCompleted:
                return OsStatusSuccess;
            default:
                KmDebugMessage("Invalid state: ", range, ": ", status, "\n");
                KM_ASSERT(false);
            }
        }
    } else if (end != mSegments.end()) {
        MemorySegment& front = begin->second;
        MemorySegment& back = end->second;
        km::MemoryRange lhs = front.range();
        km::MemoryRange rhs = back.range();
        if ((lhs.contains(range.front) || lhs.front == range.front) && (rhs.contains(range.back) || rhs.back == range.back)) {
            // |----lhs----|    |-------rhs------|
            //       |--------range-----|

            // |----lhs----|    |-------rhs------|
            // |--------range-----|

            // |----lhs----|    |-------rhs------|
            //         |----------range----------|

            // |----lhs----|      |-----rhs------|
            // |--------------range--------------|

            bool lhsShouldSplit = lhs.front != range.front;
            bool rhsShouldSplit = rhs.back != range.back;

            km::TlsfHeapCommandList list { &mHeap };
            km::TlsfAllocation lhsLo, lhsHi;
            km::TlsfAllocation rhsLo, rhsHi;

            if (lhsShouldSplit) {
                if (OsStatus status = list.split(front.handle, range.front, &lhsLo, &lhsHi)) {
                    return status;
                }
            }

            if (rhsShouldSplit) {
                if (OsStatus status = list.split(back.handle, range.back, &rhsLo, &rhsHi)) {
                    return status;
                }
            }

            list.commit();

            MemorySegment loSegmentLhs { uint8_t(front.owners), lhsLo };
            MemorySegment hiSegmentLhs { uint8_t(front.owners), lhsHi };
            MemorySegment loSegmentRhs { uint8_t(back.owners), rhsLo };
            MemorySegment hiSegmentRhs { uint8_t(back.owners), rhsHi };

            if (lhsShouldSplit) {
                mSegments.erase(lhs.back);

                hiSegmentLhs.owners += 1;

                addSegment(std::move(hiSegmentLhs));
                addSegment(std::move(loSegmentLhs));
            } else {
                retainEntry(lhs.back);
            }

            if (rhsShouldSplit) {
                mSegments.erase(rhs.back);

                loSegmentRhs.owners += 1;

                addSegment(std::move(loSegmentRhs));
                addSegment(std::move(hiSegmentRhs));
            } else {
                retainEntry(rhs.back);
            }

            if (lhs.back == rhs.front) {
                return OsStatusSuccess;
            }

            begin = mSegments.upper_bound(lhs.back);
            end = mSegments.upper_bound(rhs.front);

            for (auto it = begin; it != end; ++it) {
                retainEntry(it);
            }

            return OsStatusSuccess;
        } else {
            ++end;
        }
    }

    km::MemoryRange remaining = range;
    for (auto it = begin; it != end;) {
        switch (OsStatus status = retainRange(it, remaining, &remaining)) {
        case OsStatusCompleted:
            return OsStatusSuccess;
        case OsStatusMoreData:
            break;
        case OsStatusSuccess:
            updateIterators(remaining);
            it = begin;
            break;
        default:
            return status;
        }

        ++it;
    }

    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::release(km::MemoryRange range) [[clang::allocating]] {
    KM_ASSERT(range.isValid());

    Iterator begin = mSegments.lower_bound(range.front);
    Iterator end = mSegments.lower_bound(range.back);

    auto updateIterators = [&](km::MemoryRange newRange) {
        begin = mSegments.lower_bound(newRange.front);
        end = mSegments.lower_bound(newRange.back);

        if (end != mSegments.end()) {
            ++end;
        }
    };

    if (begin == mSegments.end()) {
        return OsStatusNotFound;
    }

    if (begin == end) {
        MemorySegment& segment = begin->second;
        km::MemoryRange seg = segment.range();
        if (seg == range) {
            releaseEntry(begin);
            return OsStatusSuccess;
        } else if (seg.contains(range) && !km::innerAdjacent(seg, range)) {
            // |--------seg-------|
            //       |--range--|
            std::array<km::TlsfAllocation, 3> allocations;
            std::array<km::PhysicalAddress, 2> points = {
                range.front,
                range.back,
            };

            if (OsStatus status = mHeap.splitv(segment.handle, points, allocations)) {
                return status;
            }

            MemorySegment loSegment { uint8_t(segment.owners), allocations[0] };
            MemorySegment midSegment { uint8_t(segment.owners), allocations[1] };
            MemorySegment hiSegment { uint8_t(segment.owners), allocations[2] };

            mSegments.erase(end);

            if (!releaseSegment(midSegment)) {
                addSegment(std::move(midSegment));
            }
            addSegment(std::move(loSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        } else if (seg.contains(range) && seg.front == range.front) {
            // |--------seg-------|
            // |----range----|
            return splitSegment(begin, range.back, kLow);
        } else if (seg.contains(range.front) && seg.back == range.back) {
            // |--------seg-------|
            //     |----range-----|
            return splitSegment(begin, range.front, kHigh);
        } else if (seg.front == range.back) {
            //             |----seg----|
            // |---range---|
            return OsStatusNotFound;
        } else if (seg.front > range.front && seg.contains(range.back)) {
            //    |--------seg-------|
            // |-----range-----|
            return splitSegment(begin, range.back, kLow);
        } else if (seg.front > range.front && seg.back == range.back) {
            //      |--------seg-------|
            // |---------range---------|
            releaseEntry(begin);
            return OsStatusSuccess;
        } else {
            km::MemoryRange remaining = range;
            switch (OsStatus status = releaseRange(begin, range, &remaining)) {
            case OsStatusCompleted:
                return OsStatusSuccess;
            default:
                KmDebugMessage("Invalid state: segment=", seg, ", range=", range, ": ", status, "\n");
                KM_ASSERT(false);
            }
        }
    } else if (end != mSegments.end()) {
        MemorySegment& front = begin->second;
        MemorySegment& back = end->second;
        km::MemoryRange lhs = front.range();
        km::MemoryRange rhs = back.range();
        if (lhs.back == range.front && rhs.back == range.back) {
            // |--------rhs-------|
            // |-------range------|
            if (releaseSegment(back)) {
                mSegments.erase(end);
            }
            return OsStatusSuccess;
        } else if ((lhs.contains(range.front) || lhs.front == range.front) && (rhs.contains(range.back) || rhs.back == range.back)) {
            // |----lhs----|    |-------rhs------|
            //       |--------range-----|

            // |----lhs----|    |-------rhs------|
            // |--------range-----|

            // |----lhs----|    |-------rhs------|
            //         |----------range----------|

            // |----lhs----|      |-----rhs------|
            // |--------------range--------------|

            bool lhsShouldSplit = lhs.front != range.front;
            bool rhsShouldSplit = rhs.back != range.back;

            km::TlsfHeapCommandList list { &mHeap };
            km::TlsfAllocation lhsLo, lhsHi;
            km::TlsfAllocation rhsLo, rhsHi;

            if (lhsShouldSplit) {
                if (OsStatus status = list.split(front.handle, range.front, &lhsLo, &lhsHi)) {
                    return status;
                }
            }

            if (rhsShouldSplit) {
                if (OsStatus status = list.split(back.handle, range.back, &rhsLo, &rhsHi)) {
                    return status;
                }
            }

            list.commit();

            MemorySegment loSegmentLhs { uint8_t(front.owners), lhsLo };
            MemorySegment hiSegmentLhs { uint8_t(front.owners), lhsHi };
            MemorySegment loSegmentRhs { uint8_t(back.owners), rhsLo };
            MemorySegment hiSegmentRhs { uint8_t(back.owners), rhsHi };

            if (lhsShouldSplit) {
                mSegments.erase(lhs.back);

                if (!releaseSegment(hiSegmentLhs)) {
                    addSegment(std::move(hiSegmentLhs));
                }

                addSegment(std::move(loSegmentLhs));
            } else {
                releaseEntry(lhs.back);
            }

            if (rhsShouldSplit) {
                mSegments.erase(rhs.back);

                if (!releaseSegment(loSegmentRhs)) {
                    addSegment(std::move(loSegmentRhs));
                }

                addSegment(std::move(hiSegmentRhs));
            } else {
                releaseEntry(rhs.back);
            }

            if (lhs.back == rhs.front) {
                return OsStatusSuccess;
            }

            begin = mSegments.upper_bound(lhs.back);
            end = mSegments.upper_bound(rhs.front);

            for (auto it = begin; it != end; ++it) {
                releaseEntry(it);
            }

            return OsStatusSuccess;
        } else if (lhs.back == range.front) {
            KM_ASSERT(rhs.contains(range.back));
            // |------rhs-------|
            // |----range----|

            return splitSegment(end, range.back, kLow);
        } else {
            ++end;
        }
    }

    // Handle more edge cases
    if (begin != mSegments.end() && end == mSegments.end()) {
        auto& lhs = begin->second;
        auto lhsRange = lhs.range();
        if (lhsRange.back == range.front) {
            // |---lhs---|
            //           |----range----|
            return OsStatusNotFound;
        }
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

OsStatus sys2::MemoryManager::allocate(size_t size, size_t align, km::MemoryRange *range) [[clang::allocating]] {
    km::TlsfAllocation allocation = mHeap.aligned_alloc(align, size);
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

    *manager = MemoryManager(std::move(heap));
    return OsStatusSuccess;
}

OsStatus sys2::MemoryManager::create(km::TlsfHeap&& heap, MemoryManager *manager) {
    *manager = MemoryManager(std::move(heap));
    return OsStatusSuccess;
}

sys2::MemoryManagerStats sys2::MemoryManager::stats() noexcept [[clang::nonallocating]] {
    DIAGNOSTIC_BEGIN_IGNORE("-Wfunction-effects");

    return MemoryManagerStats {
        .heapStats = mHeap.stats(),
        .segments = mSegments.size(),
    };

    DIAGNOSTIC_END_IGNORE();
}

OsStatus sys2::MemoryManager::querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]] {
    DIAGNOSTIC_BEGIN_IGNORE("-Wfunction-effects");

    auto it = mSegments.upper_bound(address);
    if (it == mSegments.end()) {
        return OsStatusNotFound;
    }

    auto& segment = it->second;
    auto result = segment.stats();
    bool found = segment.handle.range().contains(address);
    if (!found) {
        return OsStatusNotFound;
    }

    *stats = result;
    return OsStatusSuccess;

    DIAGNOSTIC_END_IGNORE();
}
