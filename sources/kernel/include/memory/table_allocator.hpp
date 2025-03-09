#pragma once

#include "arch/paging.hpp"
#include "memory/range.hpp"

#include "memory/detail/control_block.hpp"

namespace km {
    class PageTableAllocator;

    namespace detail {
        void *AllocateBlock(PageTableAllocator& allocator, size_t blocks);
    }

    /// @brief An allocator for page tables.
    ///
    /// An allocator for page tables that allows partially freeing larger allocations.
    class PageTableAllocator {
        VirtualRange mMemory;
        size_t mBlockSize;
        detail::ControlBlock *mHead;

        void defragmentUnlocked();

        friend void *detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks);
    public:
        PageTableAllocator(VirtualRange memory, size_t blockSize = x64::kPageSize);

        void *allocate(size_t blocks);
        void deallocate(void *ptr, size_t blocks);

        void defragment();
    };
}
