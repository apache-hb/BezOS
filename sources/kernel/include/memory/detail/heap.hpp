#pragma once

#include "memory/detail/pool.hpp"
#include "memory/range.hpp"

namespace km {
    namespace detail {
        static constexpr size_t kSmallBufferSize = 0x100;
        static constexpr size_t kMemoryClassShift = 7;
        static constexpr size_t kSecondLevelIndex = 5;
        static constexpr size_t kMaxMemoryClass = 64 - kMemoryClassShift;
        static constexpr size_t kSmallSizeStep = kSmallBufferSize / (1 << kSecondLevelIndex);

        static constexpr uint8_t BitScanLeading(uint32_t mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return (std::numeric_limits<uint32_t>::digits - 1) - std::countl_zero(mask);
        }

        static constexpr uint8_t BitScanTrailing(uint32_t mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return std::countr_zero(mask);
        }

        static constexpr uint8_t SizeToMemoryClass(size_t size) {
            if (size > kSmallBufferSize) {
                return BitScanLeading(size) - kMemoryClassShift;
            }

            return 0;
        }

        static constexpr uint16_t SizeToSecondIndex(size_t size, size_t memoryClass) {
            if (memoryClass == 0) {
                return ((size - 1) / 8);
            } else {
                return ((size >> (memoryClass + kMemoryClassShift - kSecondLevelIndex)) ^ (1u << kSecondLevelIndex));
            }
        }

        static constexpr size_t GetFreeListSize(size_t memoryClass, size_t secondIndex) {
            if (memoryClass == 0) {
                return 0;
            }

            return (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex + 1 + (1 << kSecondLevelIndex);
        }

        static constexpr size_t GetListIndex(size_t memoryClass, size_t secondIndex) {
            if (memoryClass == 0) {
                return secondIndex;
            }

            size_t index = (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex;
            return index + (1 << kSecondLevelIndex);
        }

        static constexpr size_t GetListIndex(size_t size) {
            size_t memoryClass = SizeToMemoryClass(size);
            size_t secondIndex = SizeToSecondIndex(size, memoryClass);
            return GetListIndex(memoryClass, secondIndex);
        }

        static constexpr size_t GetNextBlockSize(size_t size) {
            if (size > kSmallBufferSize) {
                return 1zu << (BitScanLeading(size) - kSecondLevelIndex);
            } else if (size > (kSmallBufferSize - kSmallSizeStep)) {
                return kSmallBufferSize + 1;
            } else {
                return size + kSmallSizeStep;
            }
        }
    }

    struct TlsfHeapStats {
        PoolAllocatorStats pool;
    };

    struct TlsfBlock {
        size_t offset;
        size_t size;
        TlsfBlock *prev;
        TlsfBlock *next;
        TlsfBlock *nextFree;
        TlsfBlock *prevFree;

        bool isFree() const noexcept [[clang::nonblocking]] {
            return (prevFree != this);
        }

        void markTaken() noexcept [[clang::nonblocking]] {
            prevFree = this;
        }

        void markFree() noexcept [[clang::nonblocking]] {
            prevFree = nullptr;
        }
    };

    class TlsfAllocation {
        TlsfBlock *block;

    public:
        constexpr TlsfAllocation() = default;

        TlsfAllocation(TlsfBlock *block) noexcept [[clang::nonblocking]]
            : block(block)
        { }

        TlsfBlock *getBlock() const noexcept [[clang::nonblocking]] { return block; }
        bool isNull() const noexcept [[clang::nonblocking]] { return block == nullptr; }
        bool isValid() const noexcept [[clang::nonblocking]] { return block != nullptr; }

        constexpr auto operator<=>(const TlsfAllocation&) const noexcept [[clang::nonblocking]] = default;
    };

    class TlsfHeap {
        using BlockPtr = TlsfBlock*;

        MemoryRange mRange;
        PoolAllocator<TlsfBlock> mBlockPool;
        TlsfBlock *mNullBlock;
        size_t mFreeListCount;
        std::unique_ptr<BlockPtr[]> mFreeList;

        size_t mMemoryClassCount;
        uint32_t mInnerFreeBitMap[detail::kMaxMemoryClass];
        uint32_t mIsFreeMap;

        TlsfBlock *findFreeBlock(size_t size, size_t *listIndex) noexcept;

        bool checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, TlsfAllocation *result);

        TlsfAllocation alloc(TlsfBlock *block, size_t size, size_t alignedOffset);

        void removeFreeBlock(TlsfBlock *block) noexcept [[clang::nonallocating]];
        void insertFreeBlock(TlsfBlock *block) noexcept [[clang::nonallocating]];
        void mergeBlock(TlsfBlock *block, TlsfBlock *prev) noexcept [[clang::nonallocating]];

        TlsfHeap(MemoryRange range, PoolAllocator<TlsfBlock>&& pool, TlsfBlock *nullBlock, size_t freeListCount, std::unique_ptr<BlockPtr[]> freeList, size_t memoryClassCount);

    public:
        UTIL_NOCOPY(TlsfHeap);
        UTIL_DEFAULT_MOVE(TlsfHeap);

        constexpr TlsfHeap() = default;
        ~TlsfHeap();

        void validate();
        void compact();

        PhysicalAddress malloc(size_t size) [[clang::allocating]];
        PhysicalAddress realloc(PhysicalAddress ptr, size_t size) [[clang::allocating]];
        TlsfAllocation aligned_alloc(size_t align, size_t size) [[clang::allocating]];
        void free(TlsfAllocation ptr) noexcept [[clang::nonallocating]];

        PhysicalAddress addressOf(TlsfAllocation ptr) const noexcept [[clang::nonblocking]] {
            if (ptr.isNull()) {
                return PhysicalAddress();
            }

            TlsfBlock *block = ptr.getBlock();
            return PhysicalAddress(block->offset + mRange.front.address);
        }

        TlsfHeapStats stats();

        static OsStatus create(MemoryRange range, TlsfHeap *heap);
    };
}
