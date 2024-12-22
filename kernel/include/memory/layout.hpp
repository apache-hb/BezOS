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

        template<typename T>
        T *as() const noexcept {
            return reinterpret_cast<T *>(address);
        }

        constexpr PhysicalAddress() noexcept = default;

        constexpr PhysicalAddress(uintptr_t address) noexcept
            : address(address)
        { }

        constexpr PhysicalAddress(std::nullptr_t) noexcept
            : address(0)
        { }

        constexpr auto operator<=>(const PhysicalAddress&) const noexcept = default;

        constexpr PhysicalAddress& operator+=(size_t offset) noexcept {
            address += offset;
            return *this;
        }

        constexpr PhysicalAddress operator+(size_t offset) const noexcept {
            return PhysicalAddress { address + offset };
        }

        constexpr PhysicalAddress& operator++() noexcept {
            address += 1;
            return *this;
        }

        constexpr PhysicalAddress operator++(int) noexcept {
            PhysicalAddress copy = *this;
            ++(*this);
            return copy;
        }
    };

    struct VirtualAddress {
        uintptr_t address;
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

        constexpr size_t size() const noexcept {
            return back.address - front.address;
        }

        constexpr bool contains(PhysicalAddress addr) const noexcept {
            return addr.address >= front.address && addr.address < back.address;
        }
    };

    constexpr size_t pages(size_t bytes) noexcept {
        return roundpow2(bytes, x64::kPageSize) / x64::kPageSize;
    }

    struct SystemMemoryLayout {
        using FreeMemoryRanges = stdx::StaticVector<MemoryRange, 32>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryRange, 32>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        void reclaimBootMemory() noexcept;

        static SystemMemoryLayout from(limine_memmap_response memmap) noexcept;
    };
}
