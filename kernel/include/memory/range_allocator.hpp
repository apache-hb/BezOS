#pragma once

#include "allocator/allocator.hpp"
#include "memory/range.hpp"
#include "std/vector.hpp"

namespace km {
    template<typename T>
    concept IsMemoryRange = requires(T it) {
        typename T::Type;
        { it.front } -> std::convertible_to<typename T::Type>;
        { it.back } -> std::convertible_to<typename T::Type>;
        { T::merge(it, it) } -> std::same_as<T>;
    };

    template<typename T>
    class RangeAllocator {
        using Range = AnyRange<T>;
        using Type = T;

        stdx::Vector<Range> mAvailable;

        void sortRanges() {
            std::sort(mAvailable.begin(), mAvailable.end(), [](const auto& a, const auto& b) {
                return a.front < b.front;
            });
        }

        void mergeRanges() {
            sortRanges();

            for (size_t i = 0; i < mAvailable.count(); i++) {
                auto& range = mAvailable[i];
                for (size_t j = i + 1; j < mAvailable.count(); j++) {
                    auto& next = mAvailable[j];
                    if (range.overlaps(next)) {
                        range = merge(range, next);
                        mAvailable.remove(j);
                        j--;
                    }
                }
            }
        }

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

            sortRanges();
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

            mergeRanges();
        }
    };
}
