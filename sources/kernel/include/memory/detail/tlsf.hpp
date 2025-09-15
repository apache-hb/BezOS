#pragma once

#include "memory/range.hpp"
#include "panic.hpp"

#include <bit>
#include <concepts>
#include <limits>

#include <stddef.h>

namespace km {
    static constexpr bool kEnableSplitVectorHardening = true;

    class TlsfAllocation;
    class TlsfHeapCommandList;
    class TlsfHeap;

    namespace detail {
        enum {
            kSmallBufferSize = 0x100,
            kMemoryClassShift = 7,
            kSecondLevelIndex = 5,
            kMaxMemoryClass = 64 - kMemoryClassShift,
            kSmallSizeStep = kSmallBufferSize / (1 << kSecondLevelIndex),
        };

        template<std::unsigned_integral T>
        constexpr uint8_t BitScanLeading(T mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return (std::numeric_limits<T>::digits - 1) - std::countl_zero(mask);
        }

        template<std::unsigned_integral T>
        constexpr uint8_t BitScanTrailing(T mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return std::countr_zero(mask);
        }

        constexpr uint8_t SizeToMemoryClass(size_t size) {
            if (size > kSmallBufferSize) {
                return BitScanLeading(size) - kMemoryClassShift;
            }

            return 0;
        }

        constexpr uint16_t SizeToSecondIndex(size_t size, uint8_t memoryClass) {
            [[assume(size > 0)]];

            if (memoryClass == 0) {
                return uint16_t((size - 1) / 8);
            } else {
                return uint16_t((size >> (memoryClass + kMemoryClassShift - kSecondLevelIndex)) ^ (1u << kSecondLevelIndex));
            }
        }

        constexpr size_t GetFreeListSize(uint8_t memoryClass, uint16_t secondIndex) {
            if (memoryClass == 0) {
                return 0;
            }

            return (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex + 1 + (1 << kSecondLevelIndex);
        }

        constexpr size_t GetListIndex(uint8_t memoryClass, uint16_t secondIndex) {
            if (memoryClass == 0) {
                return secondIndex;
            }

            size_t index = (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex;
            return index + (1 << kSecondLevelIndex);
        }

        constexpr size_t GetListIndex(size_t size) {
            [[assume(size > 0)]];

            uint8_t memoryClass = SizeToMemoryClass(size);
            uint16_t secondIndex = SizeToSecondIndex(size, memoryClass);
            return GetListIndex(memoryClass, secondIndex);
        }

        constexpr size_t GetNextBlockSize(size_t size) {
            if (size > kSmallBufferSize) {
                return 1zu << (BitScanLeading(size) - kSecondLevelIndex);
            } else if (size > (kSmallBufferSize - kSmallSizeStep)) {
                return kSmallBufferSize + 1;
            } else {
                return size + kSmallSizeStep;
            }
        }

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

            bool isUsed() const noexcept [[clang::nonblocking]] {
                return (prevFree == this);
            }

            void markTaken() noexcept [[clang::nonblocking]] {
                prevFree = this;
            }

            void markFree() noexcept [[clang::nonblocking]] {
                prevFree = nullptr;
            }

            TlsfBlock *&propNextFree() noexcept [[clang::nonblocking]] {
                KM_ASSERT(isFree());
                return nextFree;
            }
        };

        class TlsfBlockList {
            TlsfBlock *mHead;

        public:
            UTIL_NOCOPY(TlsfBlockList);

            constexpr TlsfBlockList() noexcept [[clang::nonblocking]]
                : TlsfBlockList(nullptr)
            { }

            constexpr TlsfBlockList(TlsfBlock *head) noexcept [[clang::nonblocking]]
                : mHead(head)
            { }

            constexpr TlsfBlockList(TlsfBlockList&& other) noexcept [[clang::nonblocking]]
                : mHead(std::exchange(other.mHead, nullptr))
            { }

            constexpr TlsfBlockList& operator=(TlsfBlockList&& other) noexcept [[clang::nonblocking]] {
                mHead = std::exchange(other.mHead, nullptr);
                return *this;
            }

            void add(TlsfBlock *block [[gnu::nonnull]]) noexcept [[clang::nonblocking]] {
                block->next = mHead;
                mHead = block;
            }

            void append(TlsfBlockList&& list) noexcept [[clang::nonblocking]] {
                while (TlsfBlock *block = list.drain()) {
                    add(block);
                }
            }

            [[gnu::returns_nonnull]]
            TlsfBlock *next() noexcept [[clang::nonblocking]] {
                TlsfBlock *block = mHead;
                mHead = block->next;
                block->next = nullptr;
                return block;
            }

            TlsfBlock *drain() noexcept [[clang::nonblocking]] {
                TlsfBlock *block = mHead;
                if (block != nullptr) {
                    mHead = block->next;
                    block->next = nullptr;
                }
                return block;
            }

            size_t count() const noexcept [[clang::nonallocating]] {
                size_t count = 0;
                TlsfBlock *block = mHead;
                while (block != nullptr) {
                    count++;
                    block = block->next;
                }
                return count;
            }
        };

        using TlsfBitMap = uint32_t;
    }

    class [[nodiscard]] TlsfAllocation {
        friend class TlsfHeap;

        detail::TlsfBlock *mBlock{nullptr};

        TlsfAllocation(detail::TlsfBlock *block) noexcept [[clang::nonblocking]];

    public:
        constexpr TlsfAllocation() noexcept = default;

        constexpr detail::TlsfBlock *getBlock() const noexcept [[clang::nonblocking]] { return mBlock; }
        constexpr bool isNull() const noexcept [[clang::nonblocking]] { return mBlock == nullptr; }
        constexpr bool isValid() const noexcept [[clang::nonblocking]] { return mBlock != nullptr; }

        constexpr size_t size() const noexcept [[clang::nonblocking]] {
            return mBlock->size;
        }

        constexpr PhysicalAddress address() const noexcept [[clang::nonblocking]] {
            return PhysicalAddress(mBlock->offset);
        }

        constexpr MemoryRange range() const noexcept [[clang::nonblocking]] {
            return MemoryRange::of(mBlock->offset, mBlock->size);
        }

        constexpr auto operator<=>(const TlsfAllocation&) const noexcept [[clang::nonblocking]] = default;
    };
}
