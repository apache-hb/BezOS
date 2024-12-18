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

    struct ErrorList {
        enum { eFatal = (1 << 0), eOverflow = (1 << 1) };
        struct Entry { stdx::StringView message; uint64_t entry; };
        stdx::StaticVector<Entry, 64> errors;
        unsigned flags = 0;

        void add(stdx::StringView message, uint64_t entry) {
            if (!errors.add(Entry { message, entry }))
                flags |= eOverflow;
        }

        void fatal(stdx::StringView message, uint64_t entry) {
            add(message, entry);
            flags |= eFatal;
        }

        bool isEmpty() const noexcept { return errors.isEmpty(); }
    };

    class PhysicalMemoryLayout {
        static constexpr size_t kMaxFree = 64;
        static constexpr size_t kMaxReserved = 64;
        stdx::StaticVector<MemoryRange, kMaxFree> mFreeMemory;
        stdx::StaticVector<MemoryRange, kMaxReserved> mReservedMemory;

        void setup(ErrorList& errors, const limine_memmap_response *memmap) noexcept;
        void sanitize(ErrorList& errors) noexcept;

    public:
        PhysicalMemoryLayout([[maybe_unused]] const limine_memmap_response *memmap [[gnu::nonnull]]) noexcept;
    };
}
