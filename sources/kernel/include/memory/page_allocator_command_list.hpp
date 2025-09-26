#pragma once

#include "memory/heap_command_list.hpp"
#include "memory/pmm_heap.hpp"

namespace km {
    class PageAllocator;

    class PageAllocatorCommandList {
        GenericTlsfHeapCommandList<km::PhysicalAddress> mList;

    public:
        PageAllocatorCommandList(PageAllocator *allocator [[gnu::nonnull]]) noexcept;

        [[nodiscard]]
        OsStatus split(PmmAllocation allocation, PhysicalAddress midpoint, PmmAllocation *lo [[outparam]], PmmAllocation *hi [[outparam]]) [[clang::allocating]];

        void commit() noexcept [[clang::nonallocating]];
    };
}
