#pragma once

#include <stddef.h>
#include <stdint.h>

#include "std/static_vector.hpp"
#include "std/string_view.hpp"

struct limine_memmap_response;

namespace km {
    struct PhysicalAddress { uintptr_t address; };
    struct VirtualAddress { uintptr_t address; };

    struct MemoryRange {
        uintptr_t front;
        uintptr_t back;

        constexpr size_t size() const noexcept {
            return back - front;
        }
    };

    struct SystemMemoryLayout {
        using FreeMemoryRanges = stdx::StaticVector<MemoryRange, 64>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryRange, 64>;

        FreeMemoryRanges available;
        ReservedMemoryRanges reserved;

        PhysicalAddress alloc4kPages(size_t pages) noexcept;

        static SystemMemoryLayout from(const limine_memmap_response *memmap [[gnu::nonnull]]) noexcept;
    };
}
