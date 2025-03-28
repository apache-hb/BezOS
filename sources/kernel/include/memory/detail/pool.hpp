#pragma once

#include "log.hpp"
#include "std/vector.hpp"
#include "util/util.hpp"

namespace km::detail {
    template<typename T>
    class PoolAllocator {
        union Item {
            uint32_t next;
            alignas(T) char data[sizeof(T)];
        };

        struct Block {
            uint32_t count;
            uint32_t firstFreeIndex;
            Item items[];

            void *take() {
                if (firstFreeIndex < count) {
                    Item *item = &items[firstFreeIndex];
                    firstFreeIndex = item->next;
                    return item->data;
                }

                return nullptr;
            }

            void give(Item *item) {
                item->next = firstFreeIndex;
                firstFreeIndex = std::bit_cast<uint32_t>(item - items);
            }

            bool contains(Item *item) const {
                return item >= items && item < items + count;
            }

            bool reclaim(void *ptr) {
                Item *item = std::bit_cast<Item*>(ptr);
                if (contains(item)) {
                    give(item);
                    return true;
                }

                return false;
            }
        };

        stdx::Vector2<Block*> mBlocks;

        Block *newBlock() {
            size_t nextCapacity = mBlocks.isEmpty() ? 16 : mBlocks.back().capacity() * 3 / 2;
            Block *block = new (std::nothrow) char[sizeof(Block) + (sizeof(Item) * nextCapacity)];
            if (block == nullptr) {
                return nullptr;
            }

            if (mBlocks.add(block) != OsStatusSuccess) {
                delete[] (char*)block;
                return nullptr;
            }

            block->count = nextCapacity;
            block->firstFreeIndex = 0;

            for (size_t i = 0; i < nextCapacity - 1; i++) {
                block->items[i].next = i + 1;
            }
            block->items[nextCapacity - 1].next = UINT32_MAX;

            return block;
        }

    public:
        UTIL_NOCOPY(PoolAllocator);

        PoolAllocator() = default;
        ~PoolAllocator() { clear(); }

        void clear() {
            for (Block *block : mBlocks) {
                delete[] (char*)block;
            }

            mBlocks.clear();
        }

        void *malloc() {
            for (size_t i = mBlocks.count(); i > 0; i--) {
                Block& block = mBlocks[i - 1];
                if (void *ptr = block.take()) {
                    return ptr;
                }
            }

            if (Block *block = newBlock()) {
                return block->take();
            }

            return nullptr;
        }

        void free(void *ptr) {
            for (size_t i = mBlocks.count(); i > 0; i--) {
                Block& block = mBlocks[i - 1];
                if (block.reclaim(ptr)) {
                    return;
                }
            }

            KmDebugMessage("[MEM] Freeing invalid pointer: ", ptr, "\n");
            KM_PANIC("Freeing invalid pointer.");
        }

        template<typename... Args>
        T *construct(Args&&... args) {
            if (void *ptr = malloc(sizeof(T))) {
                return new (ptr) T(std::forward<Args>(args)...);
            }

            return nullptr;
        }

        void destroy(T *ptr) {
            if (ptr != nullptr) {
                ptr->~T();
                free(ptr);
            }
        }
    };
}
