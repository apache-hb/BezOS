#include "system/pmm.hpp"

#include "memory/page_allocator.hpp"
#include "memory/page_allocator_command_list.hpp"

OsStatus sys::MemoryManager::retainRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) {
    auto& segment = it->second;
    km::MemoryRange seg = segment.allocation.range();

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

void sys::MemoryManager::retainEntry(Iterator it) {
    auto& segment = it->second;
    segment.owners += 1;
}

void sys::MemoryManager::retainEntry(km::PhysicalAddress address) {
    auto it = segments().find(address);
    KM_ASSERT(it != segments().end());
    retainEntry(it);
}

bool sys::MemoryManager::releaseEntry(km::PhysicalAddress address) {
    auto it = segments().find(address);
    KM_ASSERT(it != segments().end());
    return releaseEntry(it);
}

bool sys::MemoryManager::releaseEntry(Iterator it) {
    auto& segment = it->second;
    if (releaseSegment(segment)) {
        segments().erase(it);
        return true;
    }

    return false;
}

bool sys::MemoryManager::releaseSegment(sys::MemorySegment& segment) {
    if (segment.owners.fetch_sub(1) == 1) {
        mHeap->free(segment.allocation);
        return true;
    }
    return false;
}

void sys::MemoryManager::addSegment(MemorySegment&& segment) {
    mTable.insert(std::move(segment));
}

OsStatus sys::MemoryManager::retainSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap->split(segment.allocation, midpoint, &lo, &hi)) {
        return status;
    }

    auto loSegment = MemorySegment { uint8_t(segment.owners), lo };
    auto hiSegment = MemorySegment { uint8_t(segment.owners), hi };

    mTable.erase(it);

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

OsStatus sys::MemoryManager::splitSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) {
    auto& segment = it->second;

    km::TlsfAllocation lo, hi;
    if (OsStatus status = mHeap->split(segment.allocation, midpoint, &lo, &hi)) {
        return status;
    }

    auto loSegment = MemorySegment { uint8_t(segment.owners), lo };
    auto hiSegment = MemorySegment { uint8_t(segment.owners), hi };

    segments().erase(it);

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

OsStatus sys::MemoryManager::releaseRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) {
    auto& segment = it->second;
    km::MemoryRange seg = segment.allocation.range();
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

OsStatus sys::MemoryManager::retain(km::MemoryRange range) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    auto [begin, end] = mTable.find(range);

    auto updateIterators = [&](km::MemoryRange newRange) {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

        begin = segments().lower_bound(newRange.front);
        end = segments().lower_bound(newRange.back);

        if (end != segments().end()) {
            ++end;
        }

        CLANG_DIAGNOSTIC_POP();
    };

    if (begin == mTable.end()) {
        // No segments in the range
        return OsStatusNotFound;
    }

    if (begin == end) {
        MemorySegment& segment = begin->second;
        km::MemoryRange seg = segment.range();
        if (range.contains(seg) || seg == range) {
            // |--------seg-------|
            // |--------range-----|
            //
            //    |----seg----|
            // |--------range-----|
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

            if (OsStatus status = mHeap->splitv(segment.allocation, points, allocations)) {
                return status;
            }

            MemorySegment loSegment { uint8_t(segment.owners), allocations[0] };
            MemorySegment midSegment { uint8_t(segment.owners + 1), allocations[1] };
            MemorySegment hiSegment { uint8_t(segment.owners), allocations[2] };

            mTable.erase(begin);

            addSegment(std::move(loSegment));
            addSegment(std::move(midSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        } else if (seg.front >= range.front && seg.back > range.back) {
            // |--------seg-------|
            // |----range----|
            //
            //      |--------seg-------|
            // |----range----|
            return retainSegment(begin, range.back, kLow);
        } else if (range.front > seg.front && range.back >= seg.back) {
            // |--------seg-------|
            //     |----range-----|
            //
            // |--------seg-------|
            //       |--------range---------|
            return retainSegment(begin, range.front, kHigh);
        } else {
            KM_ASSERT(false);
        }
    } else if (end != segments().end()) {
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

            km::PageAllocatorCommandList list { mHeap };
            km::TlsfAllocation lhsLo, lhsHi;
            km::TlsfAllocation rhsLo, rhsHi;

            if (lhsShouldSplit) {
                if (OsStatus status = list.split(front.allocation, range.front, &lhsLo, &lhsHi)) {
                    return status;
                }
            }

            if (rhsShouldSplit) {
                if (OsStatus status = list.split(back.allocation, range.back, &rhsLo, &rhsHi)) {
                    return status;
                }
            }

            list.commit();

            MemorySegment loSegmentLhs { uint8_t(front.owners), lhsLo };
            MemorySegment hiSegmentLhs { uint8_t(front.owners), lhsHi };
            MemorySegment loSegmentRhs { uint8_t(back.owners), rhsLo };
            MemorySegment hiSegmentRhs { uint8_t(back.owners), rhsHi };

            if (lhsShouldSplit) {
                segments().erase(lhs.back);

                hiSegmentLhs.owners += 1;

                addSegment(std::move(hiSegmentLhs));
                addSegment(std::move(loSegmentLhs));
            } else {
                retainEntry(lhs.back);
            }

            if (rhsShouldSplit) {
                segments().erase(rhs.back);

                loSegmentRhs.owners += 1;

                addSegment(std::move(loSegmentRhs));
                addSegment(std::move(hiSegmentRhs));
            } else {
                retainEntry(rhs.back);
            }

            if (lhs.back == rhs.front) {
                return OsStatusSuccess;
            }

            begin = segments().upper_bound(lhs.back);
            end = segments().upper_bound(rhs.front);

            for (auto it = begin; it != end; ++it) {
                retainEntry(it);
            }

            return OsStatusSuccess;
        } else {
            ++end;
        }
    }

    if (begin != segments().end() && end == segments().end()) {
        MemorySegment& segment = begin->second;
        km::MemoryRange seg = segment.range();
        if (seg.back == range.front) {
            // |----seg----|
            //             |---range---|
            return OsStatusNotFound;
        }
    }

    km::MemoryRange remaining = range;
    for (auto it = begin; it != end;) {
        auto seg = it->second.range();
        if (!km::interval(seg, range)) {
            return OsStatusSuccess;
        }

        switch (OsStatus status = retainRange(it, range, &remaining)) {
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

OsStatus sys::MemoryManager::release(km::MemoryRange range) [[clang::allocating]] {
    KM_ASSERT(range.isValid());

    stdx::LockGuard guard(mLock);

    auto [begin, end] = mTable.find(range);

    auto updateIterators = [&](km::MemoryRange newRange) {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

        begin = segments().lower_bound(newRange.front);
        end = segments().lower_bound(newRange.back);

        if (end != segments().end()) {
            ++end;
        }

        CLANG_DIAGNOSTIC_POP();
    };

    if (begin == mTable.end()) {
        return OsStatusNotFound;
    }

    if (begin == end) {
        MemorySegment& segment = begin->second;
        km::MemoryRange seg = segment.range();
        if ((seg == range) || range.contains(seg)) {
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

            if (OsStatus status = mHeap->splitv(segment.allocation, points, allocations)) {
                return status;
            }

            MemorySegment loSegment { uint8_t(segment.owners), allocations[0] };
            MemorySegment midSegment { uint8_t(segment.owners), allocations[1] };
            MemorySegment hiSegment { uint8_t(segment.owners), allocations[2] };

            segments().erase(end);

            if (!releaseSegment(midSegment)) {
                addSegment(std::move(midSegment));
            }
            addSegment(std::move(loSegment));
            addSegment(std::move(hiSegment));

            return OsStatusSuccess;
        } else if ((seg.back == range.back && seg.front > range.front) || (seg.front == range.front && range.back > seg.back)) {
            //    |------seg-----|
            // |------range------|
            //
            // |-----seg----|
            // |------range------|
            releaseEntry(begin);
            return OsStatusSuccess;
        } else if ((seg.back > range.back && seg.front > range.front)) {
            //     |--------seg-------|
            // |----range----|
            return splitSegment(begin, range.back, kLow);
        } else if ((range.back > seg.back && range.front > seg.front)) {
            // |--------seg-------|
            //       |-----range------|
            return splitSegment(begin, range.front, kHigh);
        } else if (seg.front > range.front && seg.contains(range.back)) {
            //    |--------seg-------|
            // |-----range-----|
            return splitSegment(begin, range.back, kLow);
        } else if (seg.front > range.front && seg.back == range.back) {
            //      |--------seg-------|
            // |---------range---------|
            releaseEntry(begin);
            return OsStatusSuccess;
        } else if (seg.front == range.front && seg.contains(range.back)) {
            return splitSegment(begin, range.back, kLow);
        } else if (seg.back == range.back && seg.contains(range.front)) {
            return splitSegment(begin, range.front, kHigh);
        } else {
            KM_ASSERT(false);
        }
    } else if (end != segments().end()) {
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

            km::PageAllocatorCommandList list { mHeap };
            km::TlsfAllocation lhsLo, lhsHi;
            km::TlsfAllocation rhsLo, rhsHi;

            if (lhsShouldSplit) {
                if (OsStatus status = list.split(front.allocation, range.front, &lhsLo, &lhsHi)) {
                    return status;
                }
            }

            if (rhsShouldSplit) {
                if (OsStatus status = list.split(back.allocation, range.back, &rhsLo, &rhsHi)) {
                    return status;
                }
            }

            list.commit();

            MemorySegment loSegmentLhs { uint8_t(front.owners), lhsLo };
            MemorySegment hiSegmentLhs { uint8_t(front.owners), lhsHi };
            MemorySegment loSegmentRhs { uint8_t(back.owners), rhsLo };
            MemorySegment hiSegmentRhs { uint8_t(back.owners), rhsHi };

            if (lhsShouldSplit) {
                segments().erase(lhs.back);

                if (!releaseSegment(hiSegmentLhs)) {
                    addSegment(std::move(hiSegmentLhs));
                }

                addSegment(std::move(loSegmentLhs));
            } else {
                releaseEntry(lhs.back);
            }

            if (rhsShouldSplit) {
                segments().erase(rhs.back);

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

            begin = segments().upper_bound(lhs.back);
            end = segments().upper_bound(rhs.front);

            for (auto it = begin; it != end;) {
                if (releaseSegment(it->second)) {
                    it = segments().erase(it);
                    end = segments().upper_bound(rhs.front);
                } else {
                    ++it;
                }
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
    if (begin != segments().end() && end == segments().end()) {
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

OsStatus sys::MemoryManager::allocate(size_t size, size_t align, km::MemoryRange *range) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);

    km::TlsfAllocation allocation = mHeap->aligned_alloc(align, size);
    if (allocation.isNull()) {
        return OsStatusOutOfMemory;
    }

    auto result = allocation.range();
    auto segment = MemorySegment { allocation };

    addSegment(std::move(segment));

    *range = result;
    return OsStatusSuccess;
}

OsStatus sys::MemoryManager::create(km::PageAllocator *heap, MemoryManager *manager [[clang::noescape, gnu::nonnull]]) {
    *manager = MemoryManager(heap);
    return OsStatusSuccess;
}

sys::MemoryManagerStats sys::MemoryManager::stats() noexcept [[clang::nonallocating]] {
    DIAGNOSTIC_BEGIN_IGNORE("-Wfunction-effects");
    stdx::LockGuard guard(mLock);

    auto heapStats = mHeap->stats();
    return MemoryManagerStats {
        .heapStats = heapStats.heap,
        .segments = segments().size(),
    };

    DIAGNOSTIC_END_IGNORE();
}

OsStatus sys::MemoryManager::querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]] {
    DIAGNOSTIC_BEGIN_IGNORE("-Wfunction-effects");
    stdx::LockGuard guard(mLock);

    auto it = segments().upper_bound(address);
    if (it == segments().end()) {
        return OsStatusNotFound;
    }

    auto& segment = it->second;
    auto result = segment.stats();
    bool found = segment.allocation.range().contains(address);
    if (!found) {
        return OsStatusNotFound;
    }

    *stats = result;
    return OsStatusSuccess;

    DIAGNOSTIC_END_IGNORE();
}
