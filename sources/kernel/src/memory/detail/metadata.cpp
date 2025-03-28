#include "memory/detail/metadata.hpp"

using TlsfBlock = km::detail::TlsfBlock;
using TlsfMetadata = km::detail::TlsfMetadata;

static constexpr size_t kSmallBufferSize = 0x100;
static constexpr size_t kMemoryClassShift = 7;
static constexpr size_t kSecondLevelIndex = 5;

static constexpr size_t GetMemoryClass(size_t size) {
    if (size > kSmallBufferSize) {
        return std::countl_zero(size) - kMemoryClassShift;
    }

    return 0;
}

static constexpr size_t GetSecondIndex(size_t size, size_t memoryClass) {
    if (memoryClass == 0) {
        return ((size - 1) / 8);
    } else {
        return ((size >> (memoryClass + kMemoryClassShift - kSecondLevelIndex)) ^ (1 << kSecondLevelIndex));
    }
}

static constexpr size_t GetFreeListSize(size_t memoryClass, size_t secondIndex) {
    return (memoryClass == 0 ? 0 : (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex) + 1 + (1 << kSecondLevelIndex);
}

TlsfMetadata::TlsfMetadata(MemoryRange range) {
    size_t size = range.size();
    mNullBlock = mBlockPool.construct(TlsfBlock {
        .offset = 0,
        .size = size,
    });

    size_t memoryClass = GetMemoryClass(size);
    size_t secondIndex = GetSecondIndex(size, memoryClass);
    mFreeListSize = GetFreeListSize(memoryClass, secondIndex);
    mMemoryClassCount = memoryClass + 2;
    mFreeList = std::unique_ptr<TlsfBlock[]>(new TlsfBlock[mFreeListSize]);
}

TlsfMetadata::~TlsfMetadata() {
    mBlockPool.clear();
}
