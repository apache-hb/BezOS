#pragma once

#include <stddef.h>
#include <stdint.h>

#include "std/static_vector.hpp"

#include <limine.h>

struct limine_memmap_response;

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
    };

    struct VirtualAddress {
        uintptr_t address;
    };

    struct MemoryRange {
        PhysicalAddress front;
        PhysicalAddress back;

        constexpr size_t size() const noexcept {
            return back.address - front.address;
        }
    };

    struct SystemMemoryLayout {
        using FreeMemoryRanges = stdx::StaticVector<MemoryRange, 16>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryRange, 16>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        static SystemMemoryLayout from(limine_memmap_response memmap) noexcept;
    };
}
