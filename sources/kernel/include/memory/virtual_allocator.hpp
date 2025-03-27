#pragma once

#include "memory/range.hpp"

namespace km {
    struct VirtualPageAllocatorStats {

    };

    /// @brief Allocates memory with page size granularity.
    ///
    /// @details This allocator is used to allocate physical memory
    ///          and virtual address space. It requires an external
    ///          allocator as it cannot store control structures in
    ///          the memory it allocates from.
    class VirtualPageAllocatorBase {
        struct Block {

        };
    public:
        AnyRange<uintptr_t> allocate(size_t pages, size_t alignment);
    };
}
