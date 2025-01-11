#include "memory/virtual_allocator.hpp"

#include "arch/paging.hpp"

static void SortRanges(std::span<km::VirtualRange> ranges) {
    std::sort(ranges.begin(), ranges.end(), [](const km::VirtualRange& a, const km::VirtualRange& b) {
        return a.front < b.front;
    });
}

template<size_t N>
static void MergeRanges(stdx::StaticVector<km::VirtualRange, N>& ranges) {
    SortRanges(ranges);
    for (size_t i = 0; i < ranges.count(); i++) {
        km::VirtualRange& range = ranges[i];
        for (size_t j = i + 1; j < ranges.count(); j++) {
            km::VirtualRange& next = ranges[j];
            if (range.overlaps(next)) {
                range = km::merge(range, next);
                ranges.remove(j);
                j--;
            }
        }
    }
}

km::VirtualAllocator::VirtualAllocator(VirtualRange range)
    : mAvailable({ range })
{ }

void km::VirtualAllocator::markUsed(VirtualRange range) {
    for (size_t i = 0; i < mAvailable.count(); i++) {
        VirtualRange& available = mAvailable[i];

        if (available.contains(range)) {
            // if this range is fully contained in another range
            // then split the range
            VirtualRange front = { available.front, range.front };
            VirtualRange back = { range.back, available.back };

            mAvailable.remove(i);
            mAvailable.add(front);
            mAvailable.add(back);
            break;
        } else if (available.overlaps(range)) {
            // if this range overlaps with another range
            // then trim off the overlapping parts
            if (available.front < range.front) {
                available.back = range.front;
            } else {
                available.front = range.back;
            }

            // multiple ranges can overlap, so don't break here
        }
    }

    SortRanges(mAvailable);
}

void *km::VirtualAllocator::alloc4k(size_t count) {
    size_t size = (count * x64::kPageSize);
    for (size_t i = 0; i < mAvailable.count(); i++) {
        VirtualRange& range = mAvailable[i];
        if (range.size() >= size) {
            void *result = (void*)range.front;
            range.front = (void*)((uintptr_t)range.front + size);
            return result;
        }
    }

    return nullptr;
}

void km::VirtualAllocator::release(VirtualRange range) {
    mAvailable.add(range);

    MergeRanges(mAvailable);
}
