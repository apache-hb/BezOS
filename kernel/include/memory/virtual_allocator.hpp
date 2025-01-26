#pragma once

#include "std/vector.hpp"
#include "util/format.hpp"

#include <algorithm>

#include <stddef.h>
#include <stdint.h>

namespace km {
    /// @brief A range of virtual address space.
    ///
    /// @pre @a VirtualRange::front < @a VirtualRange::back
    struct VirtualRange {
        const void *front;
        const void *back;

        uintptr_t size() const {
            return (uintptr_t)back - (uintptr_t)front;
        }

        bool overlaps(VirtualRange other) const {
            return front < other.back && back > other.front;
        }

        bool contains(const void *addr) const {
            return addr >= front && addr < back;
        }

        bool contains(VirtualRange range) const {
            return range.front >= front && range.back <= back;
        }

        bool isEmpty() const {
            return front == back;
        }

        constexpr bool operator==(const VirtualRange& other) const = default;
        constexpr bool operator!=(const VirtualRange& other) const = default;
    };

    constexpr VirtualRange merge(VirtualRange a, VirtualRange b) {
        return { std::min(a.front, b.front), std::max(a.back, b.back) };
    }

    /// @brief Allocates virtual memory address space.
    class VirtualAllocator {
        /// @brief Ranges of memory that are still available.
        stdx::Vector<VirtualRange> mAvailable;

    public:
        VirtualAllocator(VirtualRange range, mem::IAllocator *allocator);

        void markUsed(VirtualRange range);

        void *alloc4k(size_t count);

        void release(VirtualRange range);
    };
}

template<>
struct km::Format<km::VirtualRange> {
    static void format(km::IOutStream& out, km::VirtualRange value) {
        out.format(value.front, "-", value.back);
    }
};
