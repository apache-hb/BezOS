#include "memory/table_allocator.hpp"
#include "log.hpp"

void *km::detail::AllocateBlock(PageTableAllocator& allocator, size_t blocks) {
    if (blocks == 0) {
        return nullptr;
    }

    auto *block = allocator.mHead;

    if (block == nullptr) {
        return nullptr;
    }

    if (block->blocks < blocks) {
        return nullptr;
    }

    if (block->blocks == blocks) {
        allocator.mHead = block->next;

        if (block->next)
            block->next->prev = nullptr;

        return block;
    }

    ControlBlock *next = (ControlBlock*)((uintptr_t)block + (blocks * kBlockSize));
    *next = ControlBlock {
        .address = (void*)next,
        .next = block->next,
        .prev = block->prev,
        .blocks = block->blocks - blocks
    };

    if (next->next == next) {
        next->next = next->next->next;
    }

    allocator.mHead = next->head();

    return block->address;
}

void km::detail::SwapBlocks(ControlBlock *a, ControlBlock *b) {
    std::swap(a->address, b->address);
    std::swap(a->blocks, b->blocks);
}

void km::detail::SortBlocks(ControlBlock *block) {
    ControlBlock *front = block->head();

    for (ControlBlock *it = front; it != nullptr; it = it->next) {
        for (ControlBlock *inner = it->next; inner != nullptr; inner = inner->next) {
            if (it->address > inner->address) {
                SwapBlocks(it, inner);
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

        if ((uintptr_t)block->address + (block->blocks * kBlockSize) == (uintptr_t)next->address) {
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
        .address = (void*)mMemory.front,
        .blocks = (mMemory.size()) / detail::kBlockSize
    };
}

void *km::PageTableAllocator::allocate(size_t blocks) {
    stdx::LockGuard guard(mLock);
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

    stdx::LockGuard guard(mLock);

    if (mHead)
        KM_CHECK(mHead->prev == nullptr, "Invalid head block.");

    detail::ControlBlock *block = (detail::ControlBlock*)ptr;
    *block = detail::ControlBlock {
        .address = ptr,
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
    stdx::LockGuard guard(mLock);
    defragmentUnlocked();
}
