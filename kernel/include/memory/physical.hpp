#pragma once

#include <stddef.h>
#include <stdint.h>

#include "std/static_vector.hpp"
#include "std/string_view.hpp"

struct limine_memmap_response;

namespace km {
    struct MemoryRange {
        uintptr_t front;
        uintptr_t back;

        constexpr size_t size() const noexcept {
            return back - front;
        }
    };

    class PhysicalMemoryLayout {
        static constexpr size_t kMaxFree = 64;
        static constexpr size_t kMaxReserved = 64;
        stdx::StaticVector<MemoryRange, kMaxFree> mFreeMemory;
        stdx::StaticVector<MemoryRange, kMaxReserved> mReservedMemory;

        void setup(const limine_memmap_response *memmap) noexcept;

    public:
        PhysicalMemoryLayout([[maybe_unused]] const limine_memmap_response *memmap [[gnu::nonnull]]) noexcept;
    };
}
