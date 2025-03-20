#include "memory/table_allocator.hpp"
#include "log.hpp"

/// @brief Split a block and return the newly created block
static km::detail::ControlBlock *SplitBlock(km::detail::ControlBlock *block, size_t size) {
    KM_CHECK(block->size > size, "Block is too small to split.");

    auto *next = (km::detail::ControlBlock*)((uintptr_t)block + ((block->size - size)));
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
        if (block->next) block->next->prev = nullptr;
        allocator.mHead = block->next;
        return result;
    } else {
        ControlBlock *next = SplitBlock(block, size);
        RemoveBlock(next);
        return next;
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

km::PageTableAllocator::PageTableAllocator(VirtualRange memory, size_t blockSize)
    : mMemory(memory)
    , mBlockSize(blockSize)
    , mHead(nullptr)
{
    bool valid = (memory.size() >= mBlockSize)
        && ((uintptr_t)memory.front % mBlockSize == 0)
        && ((uintptr_t)memory.back % mBlockSize == 0);

    if (!valid) {
        KmDebugMessage("PageTableAllocator: Memory range invalid. ", memory, "\n");
        KM_PANIC("PageTableAllocator: Memory range invalid.");
    }

    mHead = (detail::ControlBlock*)mMemory.front;

    *mHead = detail::ControlBlock {
        .size = mMemory.size()
    };
}

void *km::PageTableAllocator::allocate(size_t blocks) {
    if (void *result = detail::AllocateBlock(*this, (blocks * mBlockSize))) {
        return result;
    }

    //
    // If we failed to allocate we may yet still have enough memory, defragment and retry.
    //
    defragment();
    return detail::AllocateBlock(*this, (blocks * mBlockSize));
}

bool km::PageTableAllocator::allocateList(size_t blocks, detail::PageTableList *result) {
    if (blocks == 0) {
        return true;
    }

    //
    // Do an early check to see if we can allocate the blocks.
    // This is done rather than allocating as much as possible, then failing and deallocating.
    // defragmenting can be expensive, its easier to check first.
    //
    if (!detail::CanAllocateBlocks(mHead, blocks * mBlockSize)) {
        return false;
    }

    detail::ControlBlock *block = mHead;

    //
    // Early return in the case that the first block is large enough, otherwise
    // walk the freelist until we have enough pages.
    //
    if (block->size > blocks * mBlockSize) {
        detail::ControlBlock *next = SplitBlock(block, blocks * mBlockSize);
        RemoveBlock(next);
        *result = detail::PageTableList { (x64::page*)next, blocks };
        return true;
    }

    mHead = block->next;
    mHead->prev = nullptr;

    size_t remaining = (blocks * mBlockSize) - block->size;
    detail::PageTableList list { (x64::page*)block, block->size / mBlockSize };

    detail::ControlBlock *node = mHead;
    while (node != nullptr) {
        if (node->size > remaining) {
            detail::ControlBlock *next = SplitBlock(node, remaining);
            RemoveBlock(next);
            list.push((x64::page*)next, node->size / mBlockSize);
            remaining = 0;
        } else {
            mHead = node->next;
            RemoveBlock(node);
            list.push((x64::page*)node, node->size / mBlockSize);
            remaining -= node->size;
            node = mHead;
        }

        if (remaining == 0) {
            if (mHead) mHead->prev = nullptr;
            *result = list;
            return true;
        }
    }

    KM_PANIC("unreachable, preconditions were not computed correctly.");
}

void km::PageTableAllocator::deallocateList(detail::PageTableList list) {
    while (x64::page *page = list.drain()) {
        deallocate(page, 1);
    }
}

void km::PageTableAllocator::deallocate(void *ptr, size_t blocks) {
    if (blocks == 0) {
        return;
    }

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

void km::PageTableAllocator::defragment() {
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

km::PteAllocatorStats km::PageTableAllocator::stats() const {
    PteAllocatorStats stats{};

    for (detail::ControlBlock *block = mHead; block != nullptr; block = block->next) {
        size_t blockSize = block->size / mBlockSize;

        stats.freeBlocks += blockSize;
        stats.chainLength += 1;
        stats.largestBlock = std::max(stats.largestBlock, blockSize);
    }

    return stats;
}
