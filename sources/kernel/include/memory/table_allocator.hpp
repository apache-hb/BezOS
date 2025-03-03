#pragma once

#include "arch/paging.hpp"
#include "memory/range.hpp"
#include "std/spinlock.hpp"

namespace km {
    /// @brief An allocator for page tables.
    ///
    /// An allocator for page tables that allows partially freeing larger allocations.
    class PageTableAllocator {
        static constexpr auto kBlockSize = x64::kPageSize;

        struct ControlBlock {
            ControlBlock *next;
            ControlBlock *prev;
            size_t blocks;
        };

        stdx::SpinLock mLock;

        VirtualRange mMemory;
        ControlBlock *mHead;

    public:
        PageTableAllocator(VirtualRange memory);

        void *allocate(size_t blocks);
        void deallocate(void *ptr, size_t blocks);
    };
}
