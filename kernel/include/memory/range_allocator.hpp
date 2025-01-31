#pragma once

#include "allocator/allocator.hpp"
#include "memory/range.hpp"
#include "std/vector.hpp"

#include <numeric>

namespace km {
    namespace detail {
        template<typename T>
        void SortRanges(std::span<AnyRange<T>> ranges) {
            std::sort(ranges.begin(), ranges.end(), [](const AnyRange<T>& a, const AnyRange<T>& b) {
                return a.front < b.front;
            });
        }

        template<typename T>
        void MergeRanges(stdx::Vector<AnyRange<T>>& ranges) {
            SortRanges(std::span(ranges));

            for (size_t i = 0; i < ranges.count(); i++) {
                auto& range = ranges[i];
                if (range.isEmpty()) {
                    ranges.remove(i);
                    i--;
                    continue;
                }

                for (size_t j = i + 1; j < ranges.count(); j++) {
                    auto& next = ranges[j];
                    if (interval(range, next)) {
                        range = merge(range, next);
                        ranges.remove(j);
                        j--;
                    }
                }
            }
        }

        /// @brief Mark an area of memory as used.
        ///
        /// This function will mark an area of memory as used, and split
        /// any ranges that are fully contained in the used range.
        ///
        /// @warning The behavior is undefined if @p ranges are not
        ///          merged and sorted.
        ///
        /// @param ranges The ranges to mark.
        /// @param used The range that is used.
        /// @tparam T The type of the range.
        template<typename T>
        void MarkUsedArea(stdx::Vector<AnyRange<T>>& ranges, AnyRange<T> used) {
            using Range = AnyRange<T>;

            for (size_t i = 0; i < ranges.count(); i++) {
                Range& available = ranges[i];

                // Remove any ranges that would be completely covered
                // by the new used range
                if (used.contains(available)) {
                    ranges.remove(i);
                    i--;
                    continue;
                }

                if (available.contains(used)) {
                    // if this range is fully contained in another range
                    // then split the range
                    auto [left, right] = split(available, used);

                    ranges.remove(i);
                    ranges.add(left);
                    ranges.add(right);
                } else if (available.overlaps(used)) {
                    // if this range overlaps with another range
                    // then trim off the overlapping parts
                    available = available.cut(used);

                    // multiple ranges can overlap, so don't break here
                }
            }
        }

        template<typename T>
        AnyRange<T> AllocateSpace(stdx::Vector<AnyRange<T>>& ranges, size_t size) {
            using Range = AnyRange<T>;

            for (size_t i = 0; i < ranges.count(); i++) {
                Range& range = ranges[i];
                if (range.size() >= size) {
                    T front = range.front;
                    T back = (T)(std::bit_cast<uintptr_t>(range.front) + size);
                    range.front = back;
                    return Range { front, back };
                }
            }

            return Range{};
        }
    }

    template<typename T>
    class RangeAllocator {
        using Range = AnyRange<T>;
        using Type = T;

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
            detail::MarkUsedArea(mAvailable, range);
            detail::SortRanges(std::span(mAvailable));
        }

        Range allocate(size_t size) {
            return detail::AllocateSpace(mAvailable, size);
        }

        void release(Range range) {
            mAvailable.add(range);

            detail::MergeRanges(mAvailable);
        }

        size_t freeSpace() const {
            return std::accumulate(mAvailable.begin(), mAvailable.end(), size_t(0), [](size_t acc, const Range& range) {
                return acc + range.size();
            });
        }
    };
}
