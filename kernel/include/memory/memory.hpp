#pragma once

#include <compare>

#include <stddef.h>
#include <stddef.h>
#include <stdint.h>

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

    struct VirtualAddress {
        uintptr_t address;

        constexpr VirtualAddress() = default;

        constexpr VirtualAddress(uintptr_t address)
            : address(address)
        { }

        constexpr VirtualAddress(std::nullptr_t)
            : address(0)
        { }

        constexpr auto operator<=>(const VirtualAddress&) const = default;

        constexpr VirtualAddress operator+(intptr_t offset) const {
            return VirtualAddress { address + offset };
        }

        constexpr VirtualAddress& operator+=(intptr_t offset) {
            address += offset;
            return *this;
        }
    };

    template<typename T>
    struct VirtualPointer {
        T *data;
    };

    /// @brief A range of physical address space.
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

        constexpr bool isEmpty() const {
            return size() == 0;
        }

        /// @brief Checks if the given address is within the range.
        ///
        /// @param addr The address to check.
        /// @return True if the address is within the range, false otherwise.
        constexpr bool contains(PhysicalAddress addr) const {
            return addr >= front && addr < back;
        }

        /// @brief Checks if the given range overlaps with this range.
        ///
        /// Checks if the given range overlaps with this range. If one range
        /// is a subset of the other, they are not considered to overlap.
        ///
        /// @param range The range to check.
        /// @return True if the ranges overlap, false otherwise.
        constexpr bool overlaps(MemoryRange range) const {
            return front < range.back && back > range.front;
        }

        constexpr bool isBefore(PhysicalAddress addr) const {
            return back <= addr;
        }

        constexpr bool isAfter(PhysicalAddress addr) const {
            return front >= addr;
        }
    };

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
}

template<>
struct km::StaticFormat<km::PhysicalAddress> {
    static constexpr size_t kStringSize = km::kFormatSize<Hex<uintptr_t>>;
    static constexpr stdx::StringView toString(char *buffer, km::PhysicalAddress value) {
        return km::format(buffer, Hex(value.address).pad(16, '0'));
    }
};

template<>
struct km::StaticFormat<km::MemoryRange> {
    static constexpr size_t kStringSize = km::StaticFormat<km::PhysicalAddress>::kStringSize * 2 + 1;
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
