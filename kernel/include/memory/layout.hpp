#pragma once

#include <compare>

#include <stddef.h>
#include <stddef.h>
#include <stdint.h>

#include "util/math.hpp"
#include "arch/paging.hpp"
#include "std/static_vector.hpp"

#include <limine.h>

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

        constexpr size_t size() const {
            return back.address - front.address;
        }

        constexpr bool contains(PhysicalAddress addr) const {
            return addr.address >= front.address && addr.address < back.address;
        }
    };

    constexpr size_t pages(size_t bytes) {
        return roundpow2(bytes, x64::kPageSize) / x64::kPageSize;
    }

    struct SystemMemoryLayout {
        using FreeMemoryRanges = stdx::StaticVector<MemoryRange, 32>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryRange, 32>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        void reclaimBootMemory();

        static SystemMemoryLayout from(limine_memmap_response memmap);
    };
}
