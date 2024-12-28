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
    };

    template<typename T>
    struct PhysicalPointer {
        T *data;
    };

    template<typename T>
    struct VirtualPointer {
        T *data;
    };

    struct MemoryRange {
        PhysicalAddress front;
        PhysicalAddress back;

        constexpr intptr_t size() const {
            return back - front;
        }

        constexpr bool contains(PhysicalAddress addr) const {
            return addr >= front && addr < back;
        }
    };

    constexpr MemoryRange pageAligned(MemoryRange range) {
        return {sm::rounddown(range.front.address, x64::kPageSize), sm::roundup(range.back.address, x64::kPageSize)};
    }

    constexpr size_t pages(size_t bytes) {
        return sm::roundpow2(bytes, x64::kPageSize) / x64::kPageSize;
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
