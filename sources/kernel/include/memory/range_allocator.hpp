#pragma once

#include "memory/range.hpp"
#include "std/spinlock.hpp"
#include "std/vector.hpp"
#include "util/util.hpp"

#include <cstdlib>
#include <numeric>

namespace km {
    namespace detail {
        template<typename T>
        using RangeList = stdx::Vector2<AnyRange<T>>;

        template<typename T>
        void SortRanges(std::span<AnyRange<T>> ranges) {
            std::sort(ranges.begin(), ranges.end(), [](const AnyRange<T>& a, const AnyRange<T>& b) {
                return a.front < b.front;
            });
        }

        /// @brief Merges adjacent or overlapping ranges together.
        ///
        /// @pre @p ranges must be sorted.
        ///
        /// @param ranges The ranges to merge.
        /// @tparam T The type of the range point.
        template<typename T>
        void MergeRanges(RangeList<T>& ranges) {
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
        void MarkUsedArea(RangeList<T>& ranges, AnyRange<T> used) {
            using Range = AnyRange<T>;

            for (size_t i = 0; i < ranges.count(); i++) {
                Range& available = ranges[i];

                //
                // Remove any ranges that would be completely covered
                // by the new used range.
                //
                if (used.contains(available)) {
                    ranges.remove(i);
                    i--;
                    continue;
                }

                if (available.contains(used)) {
                    //
                    // If this range is fully contained in another range
                    // then split the range.
                    //
                    auto [left, right] = split(available, used);

                    ranges.remove(i);
                    ranges.add(left);
                    ranges.add(right);
                } else if (available.overlaps(used)) {
                    //
                    // If this range overlaps with another range
                    // then trim off the overlapping parts.
                    //
                    available = available.cut(used);

                    //
                    // Multiple ranges can overlap, so don't break here.
                    //
                }
            }
        }

        template<typename T>
        AnyRange<T> AllocateSpaceAligned(RangeList<T>& ranges, size_t size, size_t align) {
            using Range = AnyRange<T>;

            for (size_t i = 0; i < ranges.count(); i++) {
                Range range = ranges[i];
                if (range.size() >= size) {
                    T front = (T)sm::roundup(std::bit_cast<uintptr_t>(range.front), align);
                    T back = (T)(std::bit_cast<uintptr_t>(front) + size);

                    Range aligned { front, back };

                    // if the range is too small after aligning, skip it
                    if (aligned.back > range.back) {
                        continue;
                    }

                    // split the range
                    auto [left, right] = split(range, aligned);
                    ranges.add(left);
                    ranges.add(right);
                    ranges.remove(i);

                    return aligned;
                }
            }

            return Range{};
        }

        /// @brief Allocate the requested space if it is available.
        ///
        /// This function will attempt to allocate the requested space
        /// if it is available in the ranges. If the space is not available
        /// then an empty range is returned.
        ///
        /// @param ranges The ranges to allocate from.
        /// @param request The requested range.
        /// @tparam T The type of the range point.
        template<typename T>
        AnyRange<T> ClaimRangeIfAvailable(RangeList<T>& ranges, AnyRange<T> request) {
            using Range = AnyRange<T>;

            for (size_t i = 0; i < ranges.count(); i++) {
                //
                // This does not handle the case where ranges have not been merged
                // but adjacent ranges would fulfill the request.
                //
                Range range = ranges[i];
                if (range.contains(request)) {
                    auto [left, right] = split(range, request);
                    ranges.add(left);
                    ranges.add(right);
                    ranges.remove(i);

                    return request;
                }
            }

            return Range{};
        }

        /// @brief Returns the offset required to move a range into a given area.
        ///
        /// This function will return the offset required to move @p other
        /// into @p range.
        ///
        /// @pre @p range does not contain @p other.
        ///
        /// @param range The range to move into.
        /// @param other The range to move.
        /// @return The offset required to move @p other into @p range.
        /// @tparam T The type of the range point.
        template<typename T>
        constexpr intptr_t FitDistance(AnyRange<T> range, AnyRange<T> other) {
            if (other.front < range.front) {
                return range.front - other.front;
            } else {
                return range.back - other.back;
            }
        }

        /// @brief Allocate space based on a hint.
        ///
        /// @pre @p hint is aligned to @p align.
        template<typename T>
        AnyRange<T> AllocateSpaceHint(RangeList<T>& ranges, size_t align, AnyRange<T> hint) {
            using Range = AnyRange<T>;

            AnyRange closest = Range{};
            intptr_t closestDistance = std::numeric_limits<intptr_t>::max();
            size_t closestIdx = SIZE_MAX;

            for (size_t i = 0; i < ranges.count(); i++) {
                //
                // If the range totally contains the hint then we can split
                // and return the hint.
                //
                Range range = ranges[i];
                if (range.contains(hint)) {
                    auto [left, right] = split(range, hint);
                    ranges.add(left);
                    ranges.add(right);
                    ranges.remove(i);

                    return hint;
                }

                //
                // If the range is large enough to contain the hint then we can
                // record it as a possible candidate.
                //
                Range alignedRange = aligned(range, align);

                if (alignedRange.size() >= hint.size()) {
                    intptr_t distance = FitDistance(alignedRange, hint);
                    if (sm::magnitude(distance) < sm::magnitude(closestDistance)) {
                        closest = hint;
                        closest.front += distance;
                        closest.back += distance;

                        closestDistance = distance;
                        closestIdx = i;
                    }
                }
            }

            //
            // If a candidate range was found then use the range fitted to it
            // based on the hint.
            //
            if (closestIdx != SIZE_MAX) {
                auto [left, right] = split(ranges[closestIdx], closest);
                ranges.add(left);
                ranges.add(right);
                ranges.remove(closestIdx);

                return closest;
            }

            return Range{};
        }
    }

    template<typename T>
    struct RangeAllocateRequest {
        size_t size;
        size_t align;
        T hint;

        constexpr AnyRange<T> hintRange() const {
            T front = hint;
            T back = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + size);
            return { front, back };
        }
    };

    template<typename T>
    class RangeAllocator {
        using Range = AnyRange<T>;
        using Type = T;

        stdx::SpinLock mLock;
        stdx::Vector2<Range> mAvailable GUARDED_BY(mLock);

    public:
        RangeAllocator() = default;

        using Request = RangeAllocateRequest<T>;

        RangeAllocator(Range range) {
            mAvailable.add(range);
        }

        void reserve(Range range) {
            stdx::LockGuard guard(mLock);
            detail::MarkUsedArea(mAvailable, range);
            detail::SortRanges(std::span(mAvailable));
        }

        Range allocate(size_t size) {
            return allocate({ size, 1, T() });
        }

        Range allocate(Request request) {
            stdx::LockGuard guard(mLock);
            if (request.hint == T()) {
                return detail::AllocateSpaceAligned(mAvailable, request.size, request.align);
            } else {
                return detail::AllocateSpaceHint(mAvailable, request.align, request.hintRange());
            }
        }

        void release(Range range) {
            stdx::LockGuard guard(mLock);
            mAvailable.add(range);

            detail::MergeRanges(mAvailable);
        }

        size_t freeSpace() const {
            stdx::LockGuard guard(mLock);
            return std::accumulate(mAvailable.begin(), mAvailable.end(), size_t(0), [](size_t acc, const Range& range) {
                return acc + range.size();
            });
        }
    };
}
