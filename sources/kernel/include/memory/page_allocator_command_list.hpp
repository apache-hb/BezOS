#pragma once

#include "memory/heap_command_list.hpp"

namespace km {
    class PageAllocator;

    class PageAllocatorCommandList {
        PageAllocator *mAllocator;
        TlsfHeapCommandList mList;

    public:
        PageAllocatorCommandList(PageAllocator *allocator [[gnu::nonnull]]) noexcept;

        [[nodiscard]]
        OsStatus split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo [[gnu::nonnull]], TlsfAllocation *hi [[gnu::nonnull]]) [[clang::allocating]];

        void commit() noexcept [[clang::nonallocating]];
    };
}
