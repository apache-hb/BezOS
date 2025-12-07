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
        class PoolBlock {
            using Self = PoolBlock<T>;
            using Item = PoolItem<T>;

            uint32_t mSizeInBytes;
            uint32_t mFirstFreeIndex;
            Item mItems[];

        public:
            void *take() noexcept [[clang::nonblocking]] {
                if (mFirstFreeIndex < capacity()) {
                    Item *item = &mItems[mFirstFreeIndex];
                    mFirstFreeIndex = item->next;
                    return item->data;
                }

                return nullptr;
            }

            void give(Item *item) noexcept [[clang::nonblocking]] {
                item->next = mFirstFreeIndex;
                mFirstFreeIndex = (item - mItems);
            }

            bool contains(Item *item) const noexcept [[clang::nonblocking]] {
                return item >= mItems && item < mItems + capacity();
            }

            bool reclaim(void *ptr) noexcept [[clang::nonblocking]] {
                Item *item = std::bit_cast<Item*>(ptr);
                if (contains(item)) {
                    give(item);
                    return true;
                }

                return false;
            }

            size_t countFreeSlots() const noexcept [[clang::nonallocating]] {
                size_t result = 0;
                size_t index = mFirstFreeIndex;
                while (index < capacity()) {
                    index = mItems[index].next;
                    result++;
                }
                return result;
            }

            size_t capacity() const noexcept [[clang::nonblocking]] {
                return computeCapacity(memorySize());
            }

            size_t memorySize() const noexcept [[clang::nonblocking]] {
                return mSizeInBytes;
            }

            bool isEmpty() const noexcept [[clang::nonallocating]] {
                return countFreeSlots() == capacity();
            }

            void init(size_t bytes) noexcept [[clang::nonallocating]] {
                mSizeInBytes = bytes;
                mFirstFreeIndex = 0;

                size_t length = capacity();

                for (size_t i = 0; i < length - 1; i++) {
                    mItems[i].next = i + 1;
                }

                mItems[length - 1].next = UINT32_MAX;
            }

            static constexpr size_t blockMemorySize(size_t capacity) noexcept [[clang::nonblocking]] {
                return sizeof(Self) + (sizeof(Item) * capacity);
            }

            static constexpr size_t computeCapacity(size_t memorySize) noexcept [[clang::nonblocking]] {
                return (memorySize - sizeof(Self)) / sizeof(Item);
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
    template<typename T, typename Allocator = sm::allocator<std::byte>>
    class PoolAllocator {
        using Item = detail::PoolItem<T>;
        using Block = detail::PoolBlock<T>;

        using BlockAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Block*>;

        [[no_unique_address]] Allocator mAllocator;

        stdx::Vector2<Block*, BlockAllocator> mBlocks;

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

            //
            // TODO: I sure hope this returns aligned memory, std::allocator doesnt have an align param :(
            //
#if __cpp_lib_allocate_at_least >= 202302L
            auto [memory, actualCapacity] = mAllocator.allocate_at_least(Block::blockMemorySize(nextCapacity));
#else
            size_t actualCapacity = blockMemorySize(nextCapacity);
            std::byte *memory = mAllocator.allocate(actualCapacity);
#endif
            if (memory == nullptr) {
                return nullptr;
            }

            Block *block = std::bit_cast<Block*>(memory);

            if (mBlocks.add(block) != OsStatusSuccess) {
                freeBlock(block);
                return nullptr;
            }

            block->init(actualCapacity);

            return block;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfunction-effects" // we're fine with calling delete in a nonallocating context

        void freeBlock(Block *block) noexcept [[clang::nonallocating]] {
            std::byte *storage = std::bit_cast<std::byte*>(block);
            size_t size = block->memorySize();

            std::destroy_at(block);
            mAllocator.deallocate(storage, size);
        }

#pragma clang diagnostic pop

    public:
        UTIL_NOCOPY(PoolAllocator);

        friend void swap(PoolAllocator& lhs, PoolAllocator& rhs) noexcept {
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mBlocks, rhs.mBlocks);
        }

        constexpr PoolAllocator(Allocator allocator = Allocator{}) noexcept [[clang::nonallocating]]
            : mAllocator(allocator)
            , mBlocks(BlockAllocator{allocator})
        { }

        constexpr PoolAllocator(PoolAllocator&& other) noexcept = default;

        constexpr PoolAllocator& operator=(PoolAllocator&& other) noexcept {
            if (this != &other) {
                swap(*this, other);
            }
            return *this;
        }

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

            KM_PANIC("Attempted to release a pointer that was not allocated by this pool");
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
                size_t freeBlocks = block->countFreeSlots();
                freeSlots += freeBlocks;
                totalSlots += block->capacity();
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
