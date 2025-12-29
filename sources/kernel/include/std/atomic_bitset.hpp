#pragma once

#include <atomic>
#include <limits>

namespace sm {
    namespace detail {
        constexpr size_t requiredBitsetSize(size_t capacity) noexcept {
            return (capacity + 63) / 64;
        }

        static_assert(requiredBitsetSize(1) == 1);
        static_assert(requiredBitsetSize(64) == 1);
        static_assert(requiredBitsetSize(65) == 2);

        inline size_t atomicScanAndSet(std::atomic<uint64_t> *bits, size_t size) noexcept [[clang::nonblocking, clang::reentrant]] {
            constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

            for (size_t i = 0; i < requiredBitsetSize(size); i++) {
                uint64_t oldValue = bits[i].load(std::memory_order_acquire);

                for (size_t bit = 0; bit < kBitsPerElement; bit++) {
                    if ((i * kBitsPerElement + bit) >= size) {
                        break;
                    }

                    uint64_t mask = uint64_t{1} << bit;
                    if ((oldValue & mask) == 0) {
                        uint64_t newValue = oldValue | mask;
                        if (bits[i].compare_exchange_strong(oldValue, newValue)) {
                            return i * kBitsPerElement + bit;
                        } else {
                            //
                            // CAS failed, retry with updated oldValue
                            //
                            bit--;
                        }
                    }
                }
            }

            return (std::numeric_limits<size_t>::max)();
        }

        inline void atomicClearBit(std::atomic<uint64_t> *bits, size_t index) noexcept [[clang::nonblocking, clang::reentrant]] {
            constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

            size_t elementIndex = index / kBitsPerElement;
            size_t bitIndex = index % kBitsPerElement;

            uint64_t mask = ~(uint64_t{1} << bitIndex);
            bits[elementIndex].fetch_and(mask, std::memory_order_acq_rel);
        }
    }
}
