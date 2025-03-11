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
    defragmentUnlocked();
    return detail::AllocateBlock(*this, (blocks * mBlockSize));
}

void km::PageTableAllocator::deallocate(void *ptr, size_t blocks) {
    if (ptr == nullptr || blocks == 0) {
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

void km::PageTableAllocator::defragmentUnlocked() {
    detail::ControlBlock *block = mHead;

    //
    // Can't defragment if we've allocated everything.
    //
    if (block == nullptr || (block->next == nullptr && block->prev == nullptr)) {
        return;
    }

    detail::ControlBlock *l = block;
    while (l != nullptr) {
        KmDebugMessage("Block: ", (void*)l, " Next: ", (void*)l->next, " Prev: ", (void*)l->prev, " Size: ", l->size, "\n");
        l = l->next;
    }

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

void km::PageTableAllocator::defragment() {
    defragmentUnlocked();
}
