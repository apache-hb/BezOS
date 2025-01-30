#pragma once

#include <algorithm>
#include <compare>

#include <cstdint>

#include "util/format.hpp"

namespace km {
    struct PhysicalAddress {
        uintptr_t address;

        constexpr PhysicalAddress() = default;

        constexpr PhysicalAddress(uintptr_t address)
            : address(address)
        { }

        constexpr PhysicalAddress(std::nullptr_t)
            : address(0)
        { }

        constexpr auto operator<=>(const PhysicalAddress&) const = default;

        constexpr PhysicalAddress& operator+=(intptr_t offset) {
            address += offset;
            return *this;
        }

        constexpr PhysicalAddress operator+(intptr_t offset) const {
            return PhysicalAddress { address + offset };
        }

        constexpr intptr_t operator-(PhysicalAddress other) const {
            return address - other.address;
        }

        constexpr PhysicalAddress& operator++() {
            address += 1;
            return *this;
        }

        constexpr PhysicalAddress operator++(int) {
            PhysicalAddress copy = *this;
            ++(*this);
            return copy;
        }
    };

    /// @brief A range of address space.
    ///
    /// The terminology used for dealing with ranges is as follows:
    /// - front: The start of the range, this is the first byte in the range.
    /// - back: The end of the range, this is the first byte after the range.
    /// - size: The number of bytes in the range.
    /// - empty: A range where the front and back are the same.
    /// - contains: A range that is totally contained within another range.
    ///             Ranges that are a total subset of another range are considered to be contained.
    /// - overlaps: A range that shares some area with another range.
    /// - intersects: A range that shares any area with another range, not including touching.
    ///
    /// @pre @a AnyRange::front < @a AnyRange::back
    template<typename T>
    struct AnyRange {
        using ValueType = T;

        T front;
        T back;

        constexpr uintptr_t size() const {
            return std::bit_cast<uintptr_t>(back) - std::bit_cast<uintptr_t>(front);
        }

        constexpr bool isEmpty() const {
            return front == back;
        }

        /// @brief Checks if the given address is within the range.
        ///
        /// @param addr The address to check.
        /// @return True if the address is within the range, false otherwise.
        constexpr bool contains(ValueType addr) const {
            return addr >= front && addr < back;
        }

        /// @brief Checks if the given range is totally contained within this range.
        ///
        /// @param range The range to check.
        /// @return True if the range is contained, false otherwise.
        constexpr bool contains(AnyRange range) const {
            return range.front >= front && range.back <= back;
        }

        /// @brief Checks if the given range overlaps with this range.
        ///
        /// Checks if the given range overlaps with this range. If one range
        /// is a subset of the other, they are not considered to overlap.
        ///
        /// @param range The range to check.
        /// @return True if the ranges overlap, false otherwise.
        constexpr bool overlaps(AnyRange range) const {
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
        constexpr bool intersects(AnyRange range) const {
            return contains(range.front) || contains(range.back);
        }

        constexpr bool isBefore(ValueType address) const {
            return back <= address;
        }

        constexpr bool isAfter(ValueType address) const {
            return front >= address;
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
        constexpr AnyRange cut(AnyRange other) const {
            if (!overlaps(other)) return *this;

            if (other.front <= front) return {std::min(back, other.back), std::max(back, other.back)};
            if (other.back >= back) return {std::min(front, other.front), std::max(front, other.front)};

            return *this;
        }

        constexpr bool operator==(const AnyRange& other) const = default;
        constexpr bool operator!=(const AnyRange& other) const = default;
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
    constexpr AnyRange<T> intersection(AnyRange<T> a, AnyRange<T> b) {
        T front = std::max(a.front, b.front);
        T back = std::min(a.back, b.back);

        if (front >= back) {
            return AnyRange<T>{};
        }

        return {front, back};
    }

    template<typename T>
    constexpr AnyRange<T> merge(AnyRange<T> a, AnyRange<T> b) {
        return {std::min(a.front, b.front), std::max(a.back, b.back)};
    }

    template<typename T>
    constexpr bool adjacent(AnyRange<T> a, AnyRange<T> b) {
        return a.back == b.front || b.back == a.front;
    }

    /// @brief Splits the given memory range at the given address.
    ///
    /// @pre @a range contains @a at
    ///
    /// @param range The memory range.
    /// @param midpoint The address to split the range at.
    /// @return The two memory ranges.
    template<typename T, typename O>
    constexpr std::pair<AnyRange<T>, AnyRange<T>> split(AnyRange<T> range, O midpoint) {
        AnyRange<T> first = {range.front, midpoint};
        AnyRange<T> second = {midpoint, range.back};
        return {first, second};
    }

    template<typename T>
    constexpr std::pair<AnyRange<T>, AnyRange<T>> split(AnyRange<T> range, AnyRange<T> other) {
        AnyRange<T> first = {range.front, other.front};
        AnyRange<T> second = {other.back, range.back};
        return {first, second};
    }

    using VirtualRange = AnyRange<const void*>;
    using MemoryRange = AnyRange<PhysicalAddress>;
}

template<>
struct km::Format<km::PhysicalAddress> {
    static void format(km::IOutStream& out, km::PhysicalAddress value) {
        out.write(Hex(value.address).pad(16, '0'));
    }
};

template<typename T>
struct km::Format<km::AnyRange<T>> {
    static void format(km::IOutStream& out, km::AnyRange<T> value) {
        out.format(value.front, "-", value.back);
    }
};
