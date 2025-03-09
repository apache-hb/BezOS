#pragma once

#include "arch/paging.hpp"
#include "memory/range.hpp"

namespace km {
    class PageTableAllocator;

    namespace detail {
        static constexpr auto kBlockSize = x64::kPageSize;

        struct ControlBlock {
            ControlBlock *next;
            ControlBlock *prev;
            size_t blocks;

            ControlBlock *head() noexcept {
                ControlBlock *result = this;
                while (result->prev != nullptr) {
                    result = result->prev;
                }
                return result;
            }

            const ControlBlock *head() const noexcept {
                const ControlBlock *result = this;
                while (result->prev != nullptr) {
                    result = result->prev;
                }
                return result;
            }
        };

        void *AllocateBlock(PageTableAllocator& allocator, size_t blocks);
        void SortBlocks(ControlBlock *head);
        void MergeAdjacentBlocks(ControlBlock *head);
        void SwapAdjacentBlocks(ControlBlock *a, ControlBlock *b);
        void SwapAnyBlocks(ControlBlock *a, ControlBlock *b);
    }

    /// @brief An allocator for page tables.
    ///
    /// An allocator for page tables that allows partially freeing larger allocations.
    class PageTableAllocator {
        VirtualRange mMemory;
        detail::ControlBlock *mHead;

        void defragmentUnlocked();

        friend void *detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks);
    public:
        PageTableAllocator(VirtualRange memory);

        void *allocate(size_t blocks);
        void deallocate(void *ptr, size_t blocks);

        void defragment();
    };
}
