#pragma once

#include "std/vector.hpp"
#include "common/util/util.hpp"

namespace km {
    struct PoolAllocatorStats {
        /// @brief Number of currently allocated magazines.
        size_t magazines;

        /// @brief Number of slots currently available for allocation.
        size_t freeSlots;

        /// @brief Total number of slots currently allocated.
        size_t totalSlots;

        /// @brief Overhead of the memory used by the allocator.
        size_t controlMemory;
    };

    struct PoolCompactStats {
        size_t magazines;
        size_t slots;
    };

    namespace detail {
        template<typename T>
        union PoolItem {
            uint32_t next;
            alignas(T) char data[sizeof(T)];
        };

        template<typename T>
        struct PoolBlock {
            using Item = PoolItem<T>;

            uint32_t count;
            uint32_t firstFreeIndex;
            Item items[];

            void *take() noexcept [[clang::nonblocking]] {
                if (firstFreeIndex < count) {
                    Item *item = &items[firstFreeIndex];
                    firstFreeIndex = item->next;
                    return item->data;
                }

                return nullptr;
            }

            void give(Item *item) noexcept [[clang::nonblocking]] {
                item->next = firstFreeIndex;
                firstFreeIndex = (item - items);
            }

            bool contains(Item *item) const noexcept [[clang::nonblocking]] {
                return item >= items && item < items + count;
            }

            bool reclaim(void *ptr) noexcept [[clang::nonblocking]] {
                Item *item = std::bit_cast<Item*>(ptr);
                if (contains(item)) {
                    give(item);
                    return true;
                }

                return false;
            }

            size_t freeSlots() const noexcept [[clang::nonallocating]] {
                size_t result = 0;
                size_t index = firstFreeIndex;
                while (index < count) {
                    index = items[index].next;
                    result++;
                }
                return result;
            }

            size_t capacity() const noexcept [[clang::nonblocking]] {
                return count;
            }

            bool isEmpty() const noexcept [[clang::nonallocating]] {
                return freeSlots() == capacity();
            }

            void init(size_t blockCount) noexcept [[clang::nonallocating]] {
                count = blockCount;
                firstFreeIndex = 0;

                for (size_t i = 0; i < blockCount - 1; i++) {
                    items[i].next = i + 1;
                }

                items[blockCount - 1].next = UINT32_MAX;
            }
        };
    }

    /// @brief A pool allocator for objects of type T.
    ///
    /// Based on a class with the same name that is part of D3D12MA, this is used in a tlsf allocator
    /// with lookaside control blocks to allocate from memory that cannot be accessed by the allocator.
    /// D3D12MA uses this to allocate from graphics memory, which is not accessible from the CPU. We
    /// use this for physical memory, which is not mapped into the cpu address space.
    ///
    /// @cite D3D12MA
    template<typename T>
    class PoolAllocator {
        using Item = detail::PoolItem<T>;
        using Block = detail::PoolBlock<T>;

        stdx::Vector2<Block*> mBlocks;

        size_t nextBlockCapacity() const noexcept [[clang::nonblocking]] {
            if (mBlocks.isEmpty()) {
                return 16;
            } else {
                Block *block = mBlocks.back();
                return block->capacity() * 3 / 2;
            }
        }

        Block *newBlock() noexcept [[clang::allocating]] {
            size_t nextCapacity = nextBlockCapacity();
            char *memory = new (std::align_val_t(alignof(Block)), std::nothrow) char[sizeof(Block) + (sizeof(Item) * nextCapacity)];
            if (memory == nullptr) {
                return nullptr;
            }

            Block *block = std::bit_cast<Block*>(memory);

            if (mBlocks.add(block) != OsStatusSuccess) {
                freeBlock(block);
                return nullptr;
            }

            block->init(nextCapacity);

            return block;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfunction-effects" // we're fine with calling delete in a nonallocating context

        void freeBlock(Block *block) noexcept [[clang::nonallocating]] {
            std::destroy_at(block);
            operator delete[]((char*)block, std::align_val_t(alignof(Block)));
        }

#pragma clang diagnostic pop

    public:
        UTIL_NOCOPY(PoolAllocator);

        constexpr PoolAllocator(PoolAllocator&& other) noexcept = default;
        constexpr PoolAllocator& operator=(PoolAllocator&& other) noexcept {
            if (this != &other) {
                clear();
                mBlocks = std::move(other.mBlocks);
            }
            return *this;
        }

        constexpr PoolAllocator() noexcept [[clang::nonallocating]] = default;
        constexpr ~PoolAllocator() noexcept {
            clear();
        }

        void clear() noexcept [[clang::nonallocating]] {
            for (Block *block : mBlocks) {
                freeBlock(block);
            }

            mBlocks.clear();
        }

        void *allocate() noexcept [[clang::allocating]] {
            for (size_t i = mBlocks.count(); i > 0; i--) {
                Block *block = mBlocks[i - 1];
                if (void *ptr = block->take()) {
                    return ptr;
                }
            }

            if (Block *block = newBlock()) {
                return block->take();
            }

            return nullptr;
        }

        void release(void *ptr) noexcept [[clang::nonallocating]] {
            for (size_t i = mBlocks.count(); i > 0; i--) {
                Block *block = mBlocks[i - 1];
                if (block->reclaim(ptr)) {
                    return;
                }
            }

            std::unreachable();
        }

        template<typename... Args>
        T *construct(Args&&... args) noexcept [[clang::allocating]] {
            if (void *ptr = allocate()) {
                return new (ptr) T(std::forward<Args>(args)...);
            }

            return nullptr;
        }

        void destroy(T *ptr) noexcept [[clang::nonallocating]] {
            if (ptr != nullptr) {
                std::destroy_at(ptr);
                release(ptr);
            }
        }

        PoolCompactStats compact() noexcept [[clang::nonallocating]] {
            size_t magazines = 0;
            size_t slots = 0;

            size_t index = 0;
            while (index < mBlocks.count()) {
                Block *block = mBlocks[index];
                if (block->isEmpty()) {
                    slots += block->capacity();
                    magazines += 1;

                    freeBlock(block);
                    mBlocks.remove(index);
                } else {
                    index++;
                }
            }

            return PoolCompactStats { magazines, slots };
        }

        PoolAllocatorStats stats() const noexcept [[clang::nonallocating]] {
            size_t freeSlots = 0;
            size_t totalSlots = 0;
            size_t controlMemory = mBlocks.count() * sizeof(Block*);
            for (Block *block : mBlocks) {
                size_t freeBlocks = block->freeSlots();
                freeSlots += freeBlocks;
                totalSlots += block->count;
                controlMemory += sizeof(Block) + (sizeof(Item) * freeBlocks);
            }

            return PoolAllocatorStats {
                .magazines = mBlocks.count(),
                .freeSlots = freeSlots,
                .totalSlots = totalSlots,
                .controlMemory = controlMemory,
            };
        }
    };
}
