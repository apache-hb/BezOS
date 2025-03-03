#include "memory/table_allocator.hpp"
#include "log.hpp"

km::PageTableAllocator::PageTableAllocator(VirtualRange memory)
    : mMemory(memory)
    , mHead(nullptr)
{
    bool valid = (memory.size() >= kBlockSize)
        && ((uintptr_t)memory.front % kBlockSize == 0)
        && ((uintptr_t)memory.back % kBlockSize == 0);

    if (!valid) {
        KmDebugMessage("PageTableAllocator: Memory range invalid. ", memory, "\n");
        KM_PANIC("PageTableAllocator: Memory range invalid.");
    }

    mHead = (ControlBlock*)mMemory.front;

    *mHead = ControlBlock {
        .next = nullptr,
        .prev = nullptr,
        .blocks = (mMemory.size()) / kBlockSize
    };
}

void *km::PageTableAllocator::allocate(size_t blocks) {
    if (blocks == 0) {
        return nullptr;
    }

    stdx::LockGuard guard(mLock);
    ControlBlock *block = mHead;

    if (block == nullptr) {
        return nullptr;
    }

    if (block->blocks < blocks) {
        return nullptr;
    }

    if (block->blocks == blocks) {
        mHead = block->next;
        return block;
    }

    ControlBlock *next = (ControlBlock*)((uintptr_t)block + (blocks * kBlockSize));
    *next = ControlBlock {
        .next = block->next,
        .prev = block->prev,
        .blocks = block->blocks - blocks
    };

    mHead = next;

    return block;
}

void km::PageTableAllocator::deallocate(void *ptr, size_t blocks) {
    if (ptr == nullptr || blocks == 0) {
        return;
    }

    stdx::LockGuard guard(mLock);

    ControlBlock *block = (ControlBlock*)ptr;
    block->blocks = blocks;
    block->next = mHead;
    block->prev = (mHead == nullptr) ? nullptr : mHead->prev;

    mHead = block;
}
