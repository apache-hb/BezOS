#pragma once

#include "memory/layout.hpp"
#include "std/std.hpp"
#include "arch/paging.hpp"
#include "memory/range.hpp"

#include "memory/detail/control_block.hpp"
#include "memory/detail/table_list.hpp"

namespace km {
    class PageTableAllocator;

    namespace detail {
        void *AllocateBlock(PageTableAllocator& allocator, size_t size);
        bool CanAllocateBlocks(const ControlBlock *head, size_t size);
        detail::PageTableList AllocateHead(PageTableAllocator& allocator, size_t *remaining [[outparam]]);
    }

    struct PteAllocatorStats {
        /// @brief The granularity of blocks managed by the allocator.
        size_t blockSize;

        /// @brief The number of blocks currently available.
        size_t freeBlocks;

        /// @brief Number of freelist entries.
        size_t chainLength;

        /// @brief Size of the largest free block.
        size_t largestBlock;

        size_t getFreeSize() const noexcept [[clang::nonallocating]] {
            return freeBlocks * blockSize;
        }
    };

    /// @brief An allocator for page tables.
    ///
    /// An allocator for page tables that allows partially freeing larger allocations.
    class PageTableAllocator {
        size_t mBlockSize{x64::kPageSize};
        detail::ControlBlock *mHead{nullptr};

        friend void *detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks);
        friend detail::PageTableList detail::AllocateHead(PageTableAllocator& allocator, size_t *remaining [[outparam]]);

        void setHead(detail::ControlBlock *block) noexcept [[clang::nonblocking]];

    public:
        constexpr PageTableAllocator() noexcept [[clang::nonblocking]] = default;
        UTIL_NOCOPY(PageTableAllocator);
        UTIL_DEFAULT_MOVE(PageTableAllocator);

        [[nodiscard]]
        PageTableAllocation allocate(size_t blocks) [[clang::allocating]];

        void deallocate(void *ptr [[gnu::nonnull]], size_t blocks, uintptr_t slide = 0) noexcept [[clang::nonallocating]];

        void deallocate(PageTableAllocation allocation, size_t blocks) noexcept [[clang::nonallocating]];

        /// @brief Allocates a list of blocks rather than a single block.
        ///
        /// This allows the allocator to provide memory even when fragmentated.
        ///
        /// @post the data in @p result is undefined if this function returns false.
        ///
        /// @param blocks The number of blocks to allocate.
        /// @param[out] result The list to write the result to.
        ///
        /// @return If the allocation was successful.
        bool allocateList(size_t blocks, detail::PageTableList *result [[outparam]]) [[clang::allocating]];

        /// @brief Allocate extra blocks and append them to the provided list.
        bool allocateExtra(size_t blocks, detail::PageTableList& list) [[clang::allocating]];

        void deallocateList(detail::PageTableList list) noexcept [[clang::nonallocating]];

        void defragment() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        PteAllocatorStats stats() const noexcept [[clang::nonblocking]];

        [[nodiscard]]
        static OsStatus create(VirtualRangeEx memory, size_t blockSize, PageTableAllocator *allocator [[outparam]]) noexcept [[clang::allocating]];

        /// @brief Create a page table allocator from an address mapping.
        ///
        /// @pre @p mapping.vaddr must be non-null and aligned to @p blockSize.
        /// @pre @p mapping.paddr must be non-zero and aligned to @p blockSize.
        /// @pre @p mapping.size must be non-zero and a multiple of @p blockSize.
        /// @post @p allocator is left in an undefined state if this function returns an error.
        ///
        /// @param mapping The mapping to use for the allocator.
        /// @param blockSize The block size to use for the allocator.
        /// @param[out] allocator The allocator to write the result to.
        ///
        /// @return The status of the operation.
        /// @retval OsStatusSuccess The allocator was created successfully.
        /// @retval OsStatusInvalidInput The input parameters were invalid.
        [[nodiscard]]
        static OsStatus create(AddressMapping mapping, size_t blockSize, PageTableAllocator *allocator [[outparam]]) noexcept [[clang::allocating]];

#if __STDC_HOSTED__
        detail::ControlBlock *TESTING_getHead() {
            return mHead;
        }
#endif
    };
}
