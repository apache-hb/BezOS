#include "memory/table_allocator.hpp"
#include "log.hpp"

/// @brief Split a block and return the newly created block
static km::detail::ControlBlock *SplitBlock(km::detail::ControlBlock *block, size_t blocks) {
    KM_CHECK(block->blocks > blocks, "Block is too small to split.");

    auto *next = (km::detail::ControlBlock*)((uintptr_t)block + ((block->blocks - blocks) * km::detail::kBlockSize));
    block->blocks -= blocks;

    if (block->next) {
        block->next->prev = next;
    }

    next->next = block->next;

    block->next = next;

    next->prev = block;
    next->blocks = blocks;

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

void *km::detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks) {
    if (blocks == 0) {
        return nullptr;
    }

    auto *block = allocator.mHead;

    if (block == nullptr) {
        return nullptr;
    }

    KM_CHECK(block->prev == nullptr, "Invalid head block.");

    if (block->blocks < blocks) {
        return nullptr;
    } else if (block->blocks == blocks) {
        void *result = block;
        if (block->next) block->next->prev = nullptr;
        allocator.mHead = block->next;
        return result;
    } else {
        ControlBlock *next = SplitBlock(block, blocks);
        RemoveBlock(next);
        return next;
    }
}

static bool AreBlocksAdjacent(const km::detail::ControlBlock *a, const km::detail::ControlBlock *b) {
    if (a->next == b && b->prev == a) {
        return true;
    }

    if (b->next == a && a->prev == b) {
        return true;
    }

    return false;
}

void km::detail::SwapAdjacentBlocks(ControlBlock *a, ControlBlock *b) {
    KM_CHECK(a->next == b, "Blocks are not adjacent.");
    KM_CHECK(b->prev == a, "Blocks are not adjacent.");

    ControlBlock tmpa = *a;
    ControlBlock tmpb = *b;

    if (tmpa.prev) {
        tmpa.prev->next = b;
    }

    if (tmpb.next) {
        tmpb.next->prev = a;
    }

    a->next = tmpb.next;
    a->prev = b;

    b->next = a;
    b->prev = tmpa.prev;
}

void km::detail::SwapAnyBlocks(ControlBlock *a, ControlBlock *b) {
    if (AreBlocksAdjacent(a, b)) {
        SwapAdjacentBlocks(a, b);
    } else {
        ControlBlock tmpa = *a;
        ControlBlock tmpb = *b;

        if (tmpa.prev) {
            tmpa.prev->next = b;
        }
        b->prev = tmpa.prev;

        if (tmpa.next) {
            tmpa.next->prev = b;
        }
        b->next = tmpa.next;

        if (tmpb.prev) {
            tmpb.prev->next = a;
        }
        a->prev = tmpb.prev;

        if (tmpb.next) {
            tmpb.next->prev = a;
        }
        a->next = tmpb.next;
    }
}

void km::detail::SortBlocks(ControlBlock *block) {
    ControlBlock *front = block->head();

    for (ControlBlock *it = front; it != nullptr; it = it->next) {
        for (ControlBlock *inner = it; inner != nullptr; inner = inner->next) {
            if (it > inner) {
                SwapAnyBlocks(it, inner);
                it = inner;
            }
        }
    }
}

void km::detail::MergeAdjacentBlocks(ControlBlock *head) {
    ControlBlock *block = head;

    while (block != nullptr) {
        detail::ControlBlock *next = block->next;

        if (next == nullptr) {
            break;
        }

        if ((uintptr_t)block + (block->blocks * kBlockSize) == (uintptr_t)next) {
            block->blocks += next->blocks;
            block->next = next->next;

            if (next->next != nullptr) {
                next->next->prev = block;
            }
        } else {
            block = block->next;
        }
    }
}

km::PageTableAllocator::PageTableAllocator(VirtualRange memory)
    : mMemory(memory)
    , mHead(nullptr)
{
    bool valid = (memory.size() >= detail::kBlockSize)
        && ((uintptr_t)memory.front % detail::kBlockSize == 0)
        && ((uintptr_t)memory.back % detail::kBlockSize == 0);

    if (!valid) {
        KmDebugMessage("PageTableAllocator: Memory range invalid. ", memory, "\n");
        KM_PANIC("PageTableAllocator: Memory range invalid.");
    }

    mHead = (detail::ControlBlock*)mMemory.front;

    *mHead = detail::ControlBlock {
        .blocks = (mMemory.size()) / detail::kBlockSize
    };
}

void *km::PageTableAllocator::allocate(size_t blocks) {
    if (void *result = detail::AllocateBlock(*this, blocks)) {
        return result;
    }

    //
    // If we failed to allocate we may yet still have enough memory, defragment and retry.
    //
    defragmentUnlocked();
    return detail::AllocateBlock(*this, blocks);
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
        .blocks = blocks
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
