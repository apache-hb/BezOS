#pragma once

#include "arch/paging.hpp"
#include "memory/range.hpp"

#include "memory/detail/control_block.hpp"
#include "memory/detail/table_list.hpp"

namespace km {
    class PageTableAllocator;

    namespace detail {
        void *AllocateBlock(PageTableAllocator& allocator, size_t size);
        bool CanAllocateBlocks(const ControlBlock *head, size_t size);
        detail::PageTableList AllocateHead(PageTableAllocator& allocator, size_t *remaining);
    }

    struct PteAllocatorStats {
        /// @brief The number of blocks currently available.
        size_t freeBlocks;

        /// @brief Number of freelist entries.
        size_t chainLength;

        /// @brief Size of the largest free block.
        size_t largestBlock;
    };

    /// @brief An allocator for page tables.
    ///
    /// An allocator for page tables that allows partially freeing larger allocations.
    class PageTableAllocator {
        VirtualRange mMemory;
        size_t mBlockSize;
        detail::ControlBlock *mHead;

        friend void *detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks);
        friend detail::PageTableList detail::AllocateHead(PageTableAllocator& allocator, size_t *remaining);

        void setHead(detail::ControlBlock *block) {
            mHead = block;
            if (mHead != nullptr) {
                mHead->prev = nullptr;
            }
        }
    public:
        constexpr PageTableAllocator()
            : mMemory(VirtualRange{})
            , mBlockSize(x64::kPageSize)
            , mHead(nullptr)
        { }

        PageTableAllocator(VirtualRange memory, size_t blockSize = x64::kPageSize);

        [[gnu::malloc]]
        void *allocate(size_t blocks) [[clang::allocating]];

        [[gnu::nonnull]]
        void deallocate(void *ptr, size_t blocks) noexcept [[clang::nonallocating]];

        /// @brief Allocates a list of blocks rather than a single block.
        ///
        /// This allows the allocator to provide memory even when fragmentated.
        ///
        /// @post the data in @p result is undefined if this function returns false.
        ///
        /// @param blocks The number of blocks to allocate.
        /// @param [out] result The list to write the result to.
        ///
        /// @return If the allocation was successful.
        bool allocateList(size_t blocks, detail::PageTableList *result) [[clang::allocating]];

        /// @brief Allocate extra blocks and append them to the provided list.
        bool allocateExtra(size_t blocks, detail::PageTableList& list) [[clang::allocating]];

        void deallocateList(detail::PageTableList list) noexcept [[clang::nonallocating]];

        void defragment() noexcept [[clang::nonallocating]];

        PteAllocatorStats stats() const noexcept [[clang::nonallocating]];

        constexpr bool contains(const void *ptr) const noexcept [[clang::nonblocking]] {
            return mMemory.contains(ptr);
        }

#if __STDC_HOSTED__
        detail::ControlBlock *TESTING_getHead() {
            return mHead;
        }
#endif
    };
}
