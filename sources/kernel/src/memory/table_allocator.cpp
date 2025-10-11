#include "memory/table_allocator.hpp"
#include "logger/categories.hpp"

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
    next->slide = block->slide;

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

km::detail::ControlBlock *km::detail::AllocateBlock(PageTableAllocator& allocator, size_t size) {
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
        detail::ControlBlock *result = block;
        allocator.setHead(block->next);
        return result;
    } else {
        ControlBlock *next = SplitBlock(block, size);
        RemoveBlock(next);
        return next;
    }
}

km::detail::PageTableList km::detail::AllocateHead(PageTableAllocator& allocator, size_t *remaining [[outparam]]) {
    auto *block = allocator.mHead;
    size_t size = (*remaining * allocator.mBlockSize);

    KM_CHECK(block->prev == nullptr, "Invalid head block.");

    if (block->size > size) {
        ControlBlock *next = SplitBlock(block, size);
        RemoveBlock(next);
        *remaining = 0;
        PageTableAllocation allocation { (void*)next, next->slide };
        return detail::PageTableList { allocation, size / allocator.mBlockSize };
    } else if (block->size == size) {
        ControlBlock *result = block;
        allocator.setHead(block->next);
        *remaining = 0;
        PageTableAllocation allocation { (void*)result, result->slide };
        return detail::PageTableList { allocation, size / allocator.mBlockSize };
    } else {
        ControlBlock *result = block;
        size_t blockSize = block->size;
        *remaining -= (blockSize / allocator.mBlockSize);
        allocator.setHead(block->next);
        PageTableAllocation allocation { (void*)result, result->slide };
        return detail::PageTableList { allocation, blockSize / allocator.mBlockSize };
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

constexpr bool isPageTableAllocatorInputValid(km::VirtualRangeEx memory, size_t blockSize) {
    return (blockSize > 0) // The block size must be greater than 0.
        && (memory.size() >= blockSize) // The memory range must be at least the size of a block.
        && km::aligned(memory, blockSize) == memory; // The memory range must be aligned to the block size.
}

OsStatus km::PageTableAllocator::create(VirtualRangeEx memory, size_t blockSize, PageTableAllocator *allocator [[clang::noescape, gnu::nonnull]]) noexcept [[clang::allocating]] {
    if (!isPageTableAllocatorInputValid(memory, blockSize)) {
        return OsStatusInvalidInput;
    }

    detail::ControlBlock *head = std::bit_cast<detail::ControlBlock*>(memory.front);
    *head = detail::ControlBlock {
        .size = memory.size(),
        .slide = 0,
    };

    allocator->mBlockSize = blockSize;
    allocator->mHead = head;
    return OsStatusSuccess;
}

OsStatus km::PageTableAllocator::create(AddressMapping mapping, size_t blockSize, PageTableAllocator *allocator [[outparam]]) noexcept [[clang::allocating]] {
    if (!isPageTableAllocatorInputValid(mapping.virtualRangeEx(), blockSize)) {
        return OsStatusInvalidInput;
    }

    detail::ControlBlock *head = std::bit_cast<detail::ControlBlock*>(mapping.vaddr);
    *head = detail::ControlBlock {
        .size = mapping.size,
        .slide = mapping.slide(),
    };

    allocator->mBlockSize = blockSize;
    allocator->mHead = head;
    return OsStatusSuccess;
}

km::PageTableAllocation km::PageTableAllocator::allocate(size_t blocks) {
    if (detail::ControlBlock *result = detail::AllocateBlock(*this, (blocks * mBlockSize))) {
        return PageTableAllocation{result, result->slide};
    }

    //
    // If we failed to allocate we may yet still have enough memory, defragment and retry.
    //

    defragment();

    if (detail::ControlBlock *result = detail::AllocateBlock(*this, (blocks * mBlockSize))) {
        return PageTableAllocation{result, result->slide};
    }

    return PageTableAllocation{};
}

bool km::PageTableAllocator::allocateList(size_t blocks, detail::PageTableList *result [[outparam]]) {
    [[assume(blocks > 0)]];

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
            PageTableAllocation allocation { (void*)next, next->slide };
            list.push(allocation, remaining);
            remaining = 0;
        } else if (node->size == (remaining * mBlockSize)) {
            setHead(node->next);
            RemoveBlock(node);
            PageTableAllocation allocation { (void*)node, node->slide };
            list.push(allocation, remaining);
            remaining = 0;
        } else {
            setHead(node->next);
            RemoveBlock(node);
            PageTableAllocation allocation { (void*)node, node->slide };
            list.push(allocation, node->size / mBlockSize);
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
    while (PageTableAllocation page = list.drain()) {
        deallocate(page, 1);
    }
}

void km::PageTableAllocator::deallocate(void *ptr, size_t blocks, uintptr_t slide) noexcept [[clang::nonallocating]] {
    [[assume(blocks > 0)]];

    if (mHead)
        KM_CHECK(mHead->prev == nullptr, "Invalid head block.");

    detail::ControlBlock *block = (detail::ControlBlock*)ptr;
    *block = detail::ControlBlock {
        .next = mHead,
        .size = blocks * mBlockSize,
        .slide = slide,
    };
    if (mHead) mHead->prev = block;

    mHead = block;
}

void km::PageTableAllocator::deallocate(PageTableAllocation allocation, size_t blocks) noexcept [[clang::nonallocating]] {
    deallocate(allocation.getVirtual(), blocks, allocation.getSlide());
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
    detail::sortBlocks(block);

    //
    // Merge adjacent blocks.
    //
    block = block->head();
    detail::mergeAdjacentBlocks(block);

    //
    // Update the head.
    //
    mHead = block;
}

void km::PageTableAllocator::addMemory(VirtualRangeEx range) noexcept [[clang::nonallocating]] {
    if (range.size() == 0 || range.size() % mBlockSize != 0) {
        MemLog.fatalf("Invalid range. ", range, " block size: ", mBlockSize);
        KM_PANIC("Invalid range.");
    }

    if (alignedOut(range, mBlockSize) != range) {
        MemLog.fatalf("Range is not aligned to block size. ", range, " block size: ", mBlockSize);
        KM_PANIC("Range is not aligned to block size.");
    }

    detail::ControlBlock *block = std::bit_cast<detail::ControlBlock*>(range.front);
    *block = detail::ControlBlock {
        .next = mHead,
        .size = range.size(),
        .slide = 0,
    };

    mHead = block;

    //
    // TODO: we should probably insert the block in sorted order rather than sorting the entire list again.
    //
    defragment();
}

void km::PageTableAllocator::releaseMemory(VirtualRangeEx range) noexcept [[clang::nonallocating]] {
    defragment();

    detail::ControlBlock *block = mHead;
    while (block != nullptr) {
        auto blockRange = block->range();
        if (blockRange.isAfter(range)) {
            break;
        }

        auto overlap = km::intersection(blockRange, range);
        if (overlap.isEmpty()) {
            block = block->next;
            continue;
        }

        if (blockRange == overlap) {
            RemoveBlock(block);
            block = block->next;
        } else if (blockRange.startsWith(overlap)) {
            detail::ControlBlock *next = SplitBlock(block, overlap.size());
            RemoveBlock(block);
            block = next;
        } else if (blockRange.endsWith(overlap)) {
            SplitBlock(block, blockRange.size() - overlap.size());
            RemoveBlock(block->next);
            block = block->next;
            break;
        } else if (blockRange.contains(overlap)) {
            detail::ControlBlock *tail = SplitBlock(block, overlap.front - blockRange.front);
            detail::ControlBlock *middle = SplitBlock(block, overlap.size());
            RemoveBlock(middle);
            block = tail;
            break;
        } else {
            MemLog.fatalf("Something went horribly wrong: ", range, " overlaps ", blockRange);
            KM_PANIC("Internal error.");
        }
    }

    //
    // Walk back to the head of the list and update mHead.
    //
    while (block != nullptr && block->prev != nullptr) {
        block = block->prev;
    }

    setHead(block);
}

km::PteAllocatorStats km::PageTableAllocator::stats() const noexcept [[clang::nonblocking]] {
    PteAllocatorStats stats{};
    stats.blockSize = mBlockSize;

    for (detail::ControlBlock *block = mHead; block != nullptr; block = block->next) {
        size_t blockCount = block->size / mBlockSize;

        stats.freeBlocks += blockCount;
        stats.chainLength += 1;
        stats.largestBlock = std::max(stats.largestBlock, blockCount);
    }

    return stats;
}
