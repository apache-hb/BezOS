#pragma once

#include <compare>

#include <stddef.h>
#include <stddef.h>
#include <stdint.h>

#include "memory/virtual_allocator.hpp"
#include "panic.hpp"
#include "util/util.hpp"
#include "util/format.hpp"

#include "arch/paging.hpp"

#include <string.h>

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

    /// @brief A range of physical address space.
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
    /// @pre @a MemoryRange::front < @a MemoryRange::back
    struct MemoryRange {
        PhysicalAddress front;
        PhysicalAddress back;

        /// @brief Returns the size of the range.
        ///
        /// @return The size of the range.
        constexpr uintptr_t size() const {
            return back - front;
        }

        /// @brief Checks if the range is empty.
        ///
        /// @return True if the range is empty, false otherwise.
        constexpr bool isEmpty() const {
            return front == back;
        }

        /// @brief Checks if the given address is within the range.
        ///
        /// @param addr The address to check.
        /// @return True if the address is within the range, false otherwise.
        constexpr bool contains(PhysicalAddress addr) const {
            return addr >= front && addr < back;
        }

        /// @brief Checks if the given range is totally contained within this range.
        ///
        /// @param range The range to check.
        /// @return True if the range is contained, false otherwise.
        constexpr bool contains(MemoryRange range) const {
            return range.front >= front && range.back <= back;
        }

        /// @brief Checks if the given range overlaps with this range.
        ///
        /// Checks if the given range overlaps with this range. If one range
        /// is a subset of the other, they are not considered to overlap.
        ///
        /// @param range The range to check.
        /// @return True if the ranges overlap, false otherwise.
        constexpr bool overlaps(MemoryRange range) const {
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
        constexpr bool intersects(MemoryRange range) const {
            return contains(range.front) || contains(range.back);
        }

        constexpr bool isBefore(PhysicalAddress address) const {
            return back <= address;
        }

        constexpr bool isAfter(PhysicalAddress address) const {
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
        constexpr MemoryRange cut(MemoryRange other) const {
            if (other.back != back && other.front != front) {
                KM_CHECK(!contains(other), "Range is a subset of the other.");
            }

            if (!overlaps(other)) return *this;

            if (other.front <= front) return {std::min(back, other.back), std::max(back, other.back)};
            if (other.back >= back) return {std::min(front, other.front), std::max(front, other.front)};

            return *this;
        }

        constexpr bool operator==(const MemoryRange& other) const = default;
        constexpr bool operator!=(const MemoryRange& other) const = default;
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
    constexpr MemoryRange intersection(MemoryRange a, MemoryRange b) {
        PhysicalAddress front = std::max(a.front, b.front);
        PhysicalAddress back = std::min(a.back, b.back);

        if (front >= back) {
            return { 0uz, 0uz };
        }

        return {front, back};
    }

    /// @brief Splits the given memory range at the given address.
    ///
    /// @pre @a range contains @a at
    ///
    /// @param range The memory range.
    /// @param at The address to split the range at.
    /// @return The two memory ranges.
    constexpr std::pair<MemoryRange, MemoryRange> split(MemoryRange range, PhysicalAddress at) {
        MemoryRange first = {range.front, at};
        MemoryRange second = {at, range.back};
        return {first, second};
    }

    constexpr std::pair<MemoryRange, MemoryRange> split(MemoryRange range, MemoryRange other) {
        MemoryRange first = {range.front, other.front};
        MemoryRange second = {other.back, range.back};
        return {first, second};
    }

    constexpr bool adjacent(MemoryRange a, MemoryRange b) {
        return a.back == b.front || b.back == a.front;
    }

    constexpr MemoryRange merge(MemoryRange a, MemoryRange b) {
        return {std::min(a.front, b.front), std::max(a.back, b.back)};
    }

    /// @brief Returns a memory range that is page aligned.
    ///
    /// @param range The memory range.
    /// @return The page aligned memory range.
    constexpr MemoryRange pageAligned(MemoryRange range) {
        return {sm::rounddown(range.front.address, x64::kPageSize), sm::roundup(range.back.address, x64::kPageSize)};
    }

    /// @brief Returns the number of pages required to store the given number of bytes.
    ///
    /// @param bytes The number of bytes.
    /// @return The number of pages.
    constexpr size_t pages(size_t bytes) {
        return sm::roundup(bytes, x64::kPageSize) / x64::kPageSize;
    }

    struct AddressMapping {
        const void *vaddr;
        km::PhysicalAddress paddr;
        size_t size;

        MemoryRange physicalRange() const {
            return { paddr, paddr + size };
        }

        VirtualRange virtualRange() const {
            return { vaddr, (const char*)vaddr + size };
        }
    };
}

template<>
struct km::Format<km::AddressMapping> {
    static void format(km::IOutStream& out, km::AddressMapping value) {
        out.format(value.vaddr, " -> ", value.paddr, " (", sm::bytes(value.size), ")");
    }
};

template<>
struct km::Format<km::PhysicalAddress> {
    static constexpr size_t kStringSize = km::kFormatSize<Hex<uintptr_t>>;
    static constexpr stdx::StringView toString(char *buffer, km::PhysicalAddress value) {
        return km::format(buffer, Hex(value.address).pad(16, '0'));
    }
};

template<>
struct km::Format<km::MemoryRange> {
    static constexpr size_t kStringSize = km::Format<km::PhysicalAddress>::kStringSize * 2 + 1;
    static constexpr stdx::StringView toString(char *buffer, km::MemoryRange value) {
        char *ptr = buffer;

        char temp[km::kFormatSize<km::PhysicalAddress>];

        stdx::StringView front = km::format(temp, value.front);
        std::copy(front.begin(), front.end(), buffer);
        ptr += front.count();

        *ptr++ = '-';

        stdx::StringView back = km::format(temp, value.back);
        std::copy(back.begin(), back.end(), ptr);

        return stdx::StringView(buffer, ptr + back.count());
    }
};
