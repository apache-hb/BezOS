#pragma once

#include "allocator/allocator.hpp"
#include "std/vector.hpp"
#include "util/util.hpp"

namespace km {
    template<typename T, template <typename> typename TAllocator = mem::GlobalAllocator>
    class PoolAllocator {
        union Item {
            uint32_t next;
            alignas(T) char data[sizeof(T)];
        };

        struct Block {
            Item *items;
            uint32_t count;
            uint32_t firstFreeIndex;
        };

        [[no_unique_address]]
        TAllocator<Item> mAllocator;

        stdx::Vector2<Block, TAllocator<Block>> mBlocks;

    public:
        UTIL_NOCOPY(PoolAllocator);

        PoolAllocator() = default;
        ~PoolAllocator();

        void clear() {
            for (Block &block : mBlocks) {
                mAllocator.deallocate(block.items, block.count);
            }
            mBlocks.clear();
        }

        void *malloc(size_t count) {
            for (Block &block : mBlocks) {
                if (block.firstFreeIndex < block.count) {
                    uint32_t index = block.firstFreeIndex++;
                    return &block.items[index].data;
                }
            }
        }

        void free(void *ptr);

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
