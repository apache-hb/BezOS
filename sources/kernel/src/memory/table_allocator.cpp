#include "memory/table_allocator.hpp"

/// @brief Split a block and return the newly created block
static km::detail::ControlBlock *SplitBlock(km::detail::ControlBlock *block, size_t size) {
    KM_CHECK(block->size > size, "Block is too small to split.");

    auto *next = (km::detail::ControlBlock*)((uintptr_t)block + (block->size - size));
    block->size -= size;

    if (block->next) {
        block->next->prev = next;
    }

    next->next = block->next;

    block->next = next;

    next->prev = block;
    next->size = size;

    return next;
}

static void RemoveBlock(km::detail::ControlBlock *block) {
    if (block->prev) {
        block->prev->next = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }
}

void *km::detail::AllocateBlock(PageTableAllocator& allocator, size_t size) {
    if (size == 0) {
        return nullptr;
    }

    auto *block = allocator.mHead;

    if (block == nullptr) {
        return nullptr;
    }

    KM_CHECK(block->prev == nullptr, "Invalid head block.");

    if (block->size < size) {
        return nullptr;
    } else if (block->size == size) {
        void *result = block;
        allocator.setHead(block->next);
        return result;
    } else {
        ControlBlock *next = SplitBlock(block, size);
        RemoveBlock(next);
        return next;
    }
}

km::detail::PageTableList km::detail::AllocateHead(PageTableAllocator& allocator, size_t *remaining) {
    auto *block = allocator.mHead;
    size_t size = (*remaining * allocator.mBlockSize);

    KM_CHECK(block->prev == nullptr, "Invalid head block.");

    if (block->size > size) {
        ControlBlock *next = SplitBlock(block, size);
        RemoveBlock(next);
        *remaining = 0;
        return detail::PageTableList { (x64::page*)next, size / allocator.mBlockSize };
    } else if (block->size == size) {
        void *result = block;
        allocator.setHead(block->next);
        *remaining = 0;
        return detail::PageTableList { (x64::page*)result, size / allocator.mBlockSize };
    } else {
        void *result = block;
        size_t blockSize = block->size;
        *remaining -= (blockSize / allocator.mBlockSize);
        allocator.setHead(block->next);
        return detail::PageTableList { (x64::page*)result, blockSize / allocator.mBlockSize };
    }
}

bool km::detail::CanAllocateBlocks(const ControlBlock *head, size_t size) {
    const ControlBlock *block = head;
    size_t remaining = size;
    while (block != nullptr) {
        if (block->size >= remaining) {
            return true;
        }

        remaining -= block->size;
        block = block->next;
    }

    return false;
}

void km::PageTableAllocator::setHead(detail::ControlBlock *block) noexcept [[clang::nonblocking]] {
    mHead = block;
    if (mHead != nullptr) {
        mHead->prev = nullptr;
    }
}

constexpr bool VerifyMemoryRange(km::VirtualRangeEx memory, size_t blockSize) {
    return (blockSize > 0) // The block size must be greater than 0.
        && (memory.size() >= blockSize) // The memory range must be at least the size of a block.
        && km::aligned(memory, blockSize) == memory; // The memory range must be aligned to the block size.
}

OsStatus km::PageTableAllocator::create(VirtualRangeEx memory, size_t blockSize, PageTableAllocator *allocator [[clang::noescape, gnu::nonnull]]) noexcept [[clang::allocating]] {
    if (!VerifyMemoryRange(memory, blockSize)) {
        return OsStatusInvalidInput;
    }

    detail::ControlBlock *head = std::bit_cast<detail::ControlBlock*>(memory.front);
    *head = detail::ControlBlock {
        .size = memory.size(),
    };

    allocator->mMemory = memory;
    allocator->mBlockSize = blockSize;
    allocator->mHead = head;
    return OsStatusSuccess;
}

void *km::PageTableAllocator::allocate(size_t blocks) {
    if (void *result = detail::AllocateBlock(*this, (blocks * mBlockSize))) {
        return result;
    }

    //
    // If we failed to allocate we may yet still have enough memory, defragment and retry.
    //

    defragment();

    if (void *result = detail::AllocateBlock(*this, (blocks * mBlockSize))) {
        return result;
    }

    return nullptr;
}

bool km::PageTableAllocator::allocateList(size_t blocks, detail::PageTableList *result) {
    KM_CHECK(blocks > 0, "Invalid block count.");

    //
    // Do an early check to see if we can allocate the blocks.
    // This is done rather than allocating as much as possible, then failing and deallocating.
    // defragmenting can be expensive, its easier to check first.
    //
    if (!detail::CanAllocateBlocks(mHead, blocks * mBlockSize)) {
        return false;
    }

    size_t remaining = blocks;
    detail::PageTableList list = detail::AllocateHead(*this, &remaining);

    detail::ControlBlock *node = mHead;
    while (remaining > 0) {
        if (node->size > (remaining * mBlockSize)) {
            detail::ControlBlock *next = SplitBlock(node, remaining * mBlockSize);
            RemoveBlock(next);
            list.push((x64::page*)next, remaining);
            remaining = 0;
        } else if (node->size == (remaining * mBlockSize)) {
            setHead(node->next);
            RemoveBlock(node);
            list.push((x64::page*)node, remaining);
            remaining = 0;
        } else {
            setHead(node->next);
            RemoveBlock(node);
            list.push((x64::page*)node, node->size / mBlockSize);
            remaining -= (node->size / mBlockSize);
            node = mHead;
        }
    }

    KM_ASSERT(remaining == 0);
    *result = std::move(list);
    return true;
}

bool km::PageTableAllocator::allocateExtra(size_t blocks, detail::PageTableList& list) [[clang::allocating]] {
    detail::PageTableList extra;
    if (allocateList(blocks, &extra)) {
        list.append(std::move(extra));
        return true;
    }

    return false;
}

void km::PageTableAllocator::deallocateList(detail::PageTableList list) noexcept [[clang::nonallocating]] {
    while (x64::page *page = list.drain()) {
        deallocate(page, 1);
    }
}

void km::PageTableAllocator::deallocate(void *ptr, size_t blocks) noexcept [[clang::nonallocating]] {
    KM_ASSERT(blocks > 0);

    if (mHead)
        KM_CHECK(mHead->prev == nullptr, "Invalid head block.");

    detail::ControlBlock *block = (detail::ControlBlock*)ptr;
    *block = detail::ControlBlock {
        .next = mHead,
        .size = blocks * mBlockSize,
    };
    if (mHead) mHead->prev = block;

    mHead = block;
}

void km::PageTableAllocator::defragment() noexcept [[clang::nonallocating]] {
    detail::ControlBlock *block = mHead;

    //
    // Can't defragment if we've allocated everything.
    //
    if (block == nullptr || block->next == nullptr) {
        return;
    }

    KM_CHECK(block->prev == nullptr, "Invalid head block.");

    //
    // Sort the blocks by address.
    //
    detail::SortBlocks(block);

    //
    // Merge adjacent blocks.
    //
    block = block->head();
    detail::MergeAdjacentBlocks(block);

    //
    // Update the head.
    //
    mHead = block;
}

km::PteAllocatorStats km::PageTableAllocator::stats() const noexcept [[clang::nonblocking]] {
    PteAllocatorStats stats{};

    for (detail::ControlBlock *block = mHead; block != nullptr; block = block->next) {
        size_t blockSize = block->size / mBlockSize;

        stats.freeBlocks += blockSize;
        stats.chainLength += 1;
        stats.largestBlock = std::max(stats.largestBlock, blockSize);
    }

    return stats;
}

bool km::PageTableAllocator::contains(sm::VirtualAddress ptr) const noexcept [[clang::nonblocking]] {
    return mMemory.contains(ptr);
}
