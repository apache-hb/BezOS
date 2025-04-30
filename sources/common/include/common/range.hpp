#pragma once

#include <algorithm>
#include <compare> // IWYU pragma: keep
#include <span>

#include <stdint.h>

#include "common/util/util.hpp"

#include <cassert>

namespace sm {
    /// @brief A range of address space.
    ///
    /// Represents a range of [front, back) addresses. The range is inclusive of the front address
    /// and exclusive of the back address.
    ///
    /// The terminology used for dealing with ranges is as follows:
    /// - front: The start of the range, this is the first byte in the range.
    /// - back: The end of the range, this is the first byte after the range.
    /// - size: The number of bytes in the range.
    /// - empty: A range where the front and back are the same.
    /// - contains: A range that is totally contained within another range.
    ///             Ranges that are a total subset of another range are considered to be contained.
    /// - overlaps: A range that shares some area with another range.
    ///             This does not include ranges that are a subset of another range.
    /// - intersects: A range that shares any area with another range, not including touching.
    /// - contiguous: A range that shares any area with another range, or is adjacent to it.
    /// - adjacent: Two ranges that share no area, but are next to each other.
    /// - interval: A range that shares any area with another range, or is a subset of it.
    ///
    /// @pre @a AnyRange::front < @a AnyRange::back
    template<typename T>
    class AnyRange {
    public:
        using ValueType = T;

        T front;
        T back;

        constexpr uintptr_t size() const noexcept [[clang::nonblocking]] {
            assert(isValid());
            return std::bit_cast<uintptr_t>(back) - std::bit_cast<uintptr_t>(front);
        }

        constexpr bool isEmpty() const noexcept [[clang::nonblocking]] {
            return front == back;
        }

        constexpr bool isValid() const noexcept [[clang::nonblocking]] {
            return front <= back;
        }

        /// @brief Checks if the given address is within the range.
        ///
        /// @param addr The address to check.
        /// @return True if the address is within the range, false otherwise.
        constexpr bool contains(ValueType addr) const noexcept [[clang::nonblocking]] {
            return addr >= front && addr < back;
        }

        /// @brief Checks if the given range is totally contained within this range.
        ///
        /// @param range The range to check.
        /// @return True if the range is contained, false otherwise.
        constexpr bool contains(AnyRange range) const noexcept [[clang::nonblocking]] {
            return range.front >= front && range.back <= back;
        }

        /// @brief Checks if the given range overlaps with this range.
        ///
        /// Checks if the given range overlaps with this range. If one range
        /// is a subset of the other, they are not considered to overlap.
        ///
        /// @param range The range to check.
        /// @return True if the ranges overlap, false otherwise.
        constexpr bool overlaps(AnyRange range) const noexcept [[clang::nonblocking]] {
            if (range.front == front && range.back <= back) return true;
            if (range.back == back && range.front >= front) return true;

            return contains(range.front) ^ contains(range.back);
        }

        /// @brief Checks if the given range intersects with this range.
        ///
        /// Checks if the given range intersects with this range.
        /// If one range is a subset of the other, or if the ranges overlap,
        /// they are considered to intersect. Ranges that only touch
        /// are not considered to intersect.
        ///
        /// @param range The range to check.
        /// @return True if the ranges intersect, false otherwise.
        constexpr bool intersects(AnyRange range) const noexcept [[clang::nonblocking]] {
            if (range.front == back || range.back == front) return false;

            return contains(range) || contains(range.front) || contains(range.back) || range.back == back;
        }

        constexpr bool isBefore(ValueType address) const noexcept [[clang::nonblocking]] {
            return back <= address;
        }

        constexpr bool isBefore(AnyRange other) const noexcept [[clang::nonblocking]] {
            return back <= other.front;
        }

        constexpr bool isAfter(ValueType address) const noexcept [[clang::nonblocking]] {
            return front >= address;
        }

        constexpr bool isAfter(AnyRange other) const noexcept [[clang::nonblocking]] {
            return front >= other.back;
        }

        constexpr AnyRange offsetBy(intptr_t offset) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + offset);
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(back) + offset);
            return {newFront, newBack};
        }

        constexpr AnyRange withExtraHead(uintptr_t size) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) - size);
            return {newFront, back};
        }

        constexpr AnyRange withExtraTail(uintptr_t size) const noexcept [[clang::nonblocking]] {
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(back) + size);
            return {front, newBack};
        }

        constexpr AnyRange withoutFirst(uintptr_t size) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + size);
            assert(newFront <= back);
            return {newFront, back};
        }

        constexpr AnyRange withoutLast(uintptr_t size) const noexcept [[clang::nonblocking]] {
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(back) - size);
            assert(newBack >= front);
            return {front, newBack};
        }

        constexpr AnyRange withExtraBounds(uintptr_t extraHead, uintptr_t extraTail) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) - extraHead);
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(back) + extraTail);
            return {newFront, newBack};
        }

        constexpr AnyRange withExtraBounds(uintptr_t extra) const noexcept [[clang::nonblocking]] {
            return withExtraBounds(extra, extra);
        }

        constexpr AnyRange subrange(uintptr_t offset, uintptr_t size) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + offset);
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(newFront) + size);

            assert(newBack <= back);

            return {newFront, newBack};
        }

        constexpr AnyRange subrange(uintptr_t offset) const noexcept [[clang::nonblocking]] {
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + offset);
            assert(newFront <= back);
            return {newFront, back};
        }

        constexpr AnyRange first(uintptr_t newSize) const noexcept [[clang::nonblocking]] {
            assert(newSize <= size());
            T newBack = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + newSize);
            return {front, newBack};
        }

        constexpr AnyRange last(uintptr_t newSize) const noexcept [[clang::nonblocking]] {
            assert(newSize <= size());
            T newFront = std::bit_cast<T>(std::bit_cast<uintptr_t>(back) - newSize);
            return {newFront, back};
        }

        constexpr AnyRange mergedWith(AnyRange other) const noexcept [[clang::nonblocking]] {
            return {std::min(front, other.front), std::max(back, other.back)};
        }

        /// @brief Return a copy of this range with the overlapping area cut out.
        ///
        /// Cut off a range from this range. If there is no overlap the original range
        /// is returned. If the other range is a subset of this range, the behavior is undefined.
        ///
        /// @see km::split for a similar function that returns two ranges.
        ///
        /// @param other The range to cut out.
        /// @return The range with the overlapping area cut out.
        constexpr AnyRange cut(AnyRange other) const noexcept [[clang::nonblocking]] {
            if (!overlaps(other)) return *this;

            if (other.front <= front) return {std::min(back, other.back), std::max(back, other.back)};
            if (other.back >= back) return {std::min(front, other.front), std::max(front, other.front)};

            return *this;
        }

        constexpr std::pair<AnyRange, AnyRange> split(AnyRange other) const noexcept [[clang::nonblocking]] {
            assert(contains(other));

            AnyRange first = {front, other.front};
            AnyRange second = {other.back, back};
            return {first, second};
        }

        constexpr std::pair<AnyRange, AnyRange> split(ValueType midpoint) const noexcept [[clang::nonblocking]] {
            assert(contains(midpoint));

            AnyRange first = {front, midpoint};
            AnyRange second = {midpoint, back};
            return {first, second};
        }

        constexpr bool operator==(const AnyRange& other) const noexcept [[clang::nonblocking]] = default;
        constexpr bool operator!=(const AnyRange& other) const noexcept [[clang::nonblocking]] = default;

        constexpr static AnyRange of(T front, uintptr_t size) noexcept [[clang::nonblocking]] {
            T back = std::bit_cast<T>(std::bit_cast<uintptr_t>(front) + size);
            return {front, back};
        }

        template<typename U>
        constexpr auto cast() const noexcept [[clang::nonblocking]] {
            return AnyRange<U> { std::bit_cast<U>(front), std::bit_cast<U>(back) };
        }

        constexpr static bool create(T front, T back, AnyRange *result [[gnu::nonnull]]) noexcept [[clang::nonblocking]] {
            if (front >= back) {
                *result = AnyRange{};
                return false;
            }

            *result = AnyRange{front, back};
            return true;
        }

        constexpr static AnyRange unchecked(T front, T back) noexcept [[clang::nonblocking]] {
            return AnyRange{front, back};
        }
    };

    /// @brief Find the intersection of two ranges.
    ///
    /// Finds the intersecting area of two ranges. If one range
    /// is a subset of the other the subset is returned. If there
    /// is no overlap the empty range is returned.
    ///
    /// @param a The first memory range.
    /// @param b The second memory range.
    /// @return The intersection of the ranges.
    template<typename T>
    constexpr AnyRange<T> intersection(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        T front = std::max(a.front, b.front);
        T back = std::min(a.back, b.back);

        if (front >= back) {
            return AnyRange<T>{};
        }

        return {front, back};
    }

    template<typename T>
    constexpr AnyRange<T> merge(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return {std::min(a.front, b.front), std::max(a.back, b.back)};
    }

    /// @brief Tests if the given ranges are adjacent.
    ///
    /// Tests if the given ranges are adjacent. Adjacent ranges have a touching
    /// front or back, but do not overlap.
    ///
    /// @param a The first range.
    /// @param b The second range.
    /// @return True if the ranges are adjacent, false otherwise.
    template<typename T>
    constexpr bool outerAdjacent(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return a.back == b.front || b.back == a.front;
    }

    /// @brief Tests if the given ranges are adjacent on the inside.
    ///
    /// Tests if the given ranges are adjacent on the inside. Inner adjacent ranges
    /// have an equal front or back, and overlap.
    ///
    /// @param a The first range.
    /// @param b The second range.
    /// @return True if the ranges are adjacent on the inside, false otherwise.
    template<typename T>
    constexpr bool innerAdjacent(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return a.front == b.front || a.back == b.back;
    }

    /// @brief Test if a given range contains another range including touching borders.
    template<typename T>
    constexpr bool borderContains(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        if (a.front == b.front) {
            return a.back >= b.back;
        } else if (a.back == b.back) {
            return a.front <= b.front;
        } else {
            return a.contains(b);
        }
    }

    template<typename T>
    constexpr bool overlaps(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return a.overlaps(b);
    }

    template<typename T>
    constexpr bool contiguous(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return outerAdjacent(a, b) || overlaps(a, b);
    }

    template<typename T>
    constexpr bool interval(AnyRange<T> a, AnyRange<T> b) noexcept [[clang::nonblocking]] {
        return outerAdjacent(a, b) || overlaps(a, b) || a.contains(b) || b.contains(a);
    }

    template<typename T>
    constexpr AnyRange<T> combinedInterval(std::span<const AnyRange<T>> ranges) noexcept [[clang::nonblocking]] {
        if (ranges.empty()) return AnyRange<T>{};

        AnyRange<T> result = ranges[0];

        for (size_t i = 1; i < ranges.size(); ++i) {
            result = merge(result, ranges[i]);
        }

        return result;
    }

    /// @brief Aligns the given memory range to the given alignment.
    ///
    /// Aligns the given memory range to the given alignment. Aligns the range by shrinking
    /// the range to the next multiple of the given alignment.
    ///
    /// @param range The memory range.
    /// @param align The alignment to use.
    /// @return The aligned memory range.
    /// @tparam T The type of the range point.
    template<typename T>
    constexpr AnyRange<T> aligned(AnyRange<T> range, size_t align) noexcept [[clang::nonblocking]] {
        T front = std::bit_cast<T>(sm::roundup(std::bit_cast<uintptr_t>(range.front), align));
        T back = std::bit_cast<T>(sm::rounddown(std::bit_cast<uintptr_t>(range.back), align));

        if (front >= back) {
            return AnyRange<T>{};
        }

        return {front, back};
    }

    /// @brief Aligns the given range to the given alignment.
    ///
    /// Aligns the range by expanding its size to the next multiple of the given alignment.
    ///
    /// @param range The range to align.
    /// @param align The alignment to use.
    /// @return The aligned range.
    template<typename T>
    constexpr AnyRange<T> alignedOut(AnyRange<T> range, size_t align) noexcept [[clang::nonblocking]] {
        T front = std::bit_cast<T>(sm::rounddown(std::bit_cast<uintptr_t>(range.front), align));
        T back = std::bit_cast<T>(sm::roundup(std::bit_cast<uintptr_t>(range.back), align));

        return {front, back};
    }

    /// @brief Splits the given memory range at the given address.
    ///
    /// @pre @p range contains @p at
    ///
    /// @param range The memory range.
    /// @param midpoint The address to split the range at.
    /// @return The two memory ranges.
    template<typename T, typename O>
    constexpr std::pair<AnyRange<T>, AnyRange<T>> split(AnyRange<T> range, O midpoint) noexcept [[clang::nonblocking]] {
        assert(range.contains(midpoint));

        AnyRange<T> first = {range.front, midpoint};
        AnyRange<T> second = {midpoint, range.back};
        return {first, second};
    }

    /// @brief Split a memory range at another memory range.
    ///
    /// If @p other is not a subset of @p range, the behavior is undefined.
    /// If the beginning of @p other is the same as the beginning of @p range,
    /// the first range will be empty. If the end of @p other is the same as the
    /// end of @p range, the second range will be empty.
    ///
    /// @pre @p range contains @p other
    ///
    /// @param range The memory range to split.
    /// @param other The memory range to split at.
    /// @return The two memory ranges.
    template<typename T>
    constexpr std::pair<AnyRange<T>, AnyRange<T>> split(AnyRange<T> range, AnyRange<T> other) noexcept [[clang::nonblocking]] {
        assert(range.contains(other));

        AnyRange<T> first = {range.front, other.front};
        AnyRange<T> second = {other.back, range.back};
        return {first, second};
    }
}
