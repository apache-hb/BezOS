#pragma once

#include "allocator/allocator.hpp"
#include "std/vector.hpp"

namespace km {
    template<typename Range>
    class RangeAllocator {
        using Type = typename Range::Type;
        stdx::Vector<Range> mAvailable;

    public:
        RangeAllocator(mem::IAllocator *allocator)
            : mAvailable(allocator)
        { }

        RangeAllocator(Range range, mem::IAllocator *allocator)
            : mAvailable(allocator)
        {
            mAvailable.add(range);
        }

        void markUsed(Range range) {
            for (size_t i = 0; i < mAvailable.count(); i++) {
                Range& available = mAvailable[i];

                if (available.isEmpty()) {
                    mAvailable.remove(i);
                    i--;
                    continue;
                }

                if (available.contains(range)) {
                    // if this range is fully contained in another range
                    // then split the range
                    Range front = { available.front, range.front };
                    Range back = { range.back, available.back };

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
        }

        Range allocate(size_t size) {
            for (size_t i = 0; i < mAvailable.count(); i++) {
                Range& range = mAvailable[i];
                if (range.size() >= size) {
                    Type front = range.front;
                    Type back = (Type)((uintptr_t)range.front + size);
                    range.front = back;
                    return Range { front, back };
                }
            }

            return Range{};
        }

        void release(Range range) {
            mAvailable.add(range);
        }
    };
}