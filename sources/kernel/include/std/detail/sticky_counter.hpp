#pragma once

#include "common/util/util.hpp"

#include <atomic>

namespace sm::detail {
    /// @brief A counter that when decremented to zero will be stuck at zero.
    ///
    /// @cite AtomicSharedPtr
    template<typename T>
    class StickyCounter {
        static constexpr T kZeroFlag = T(1) << (sizeof(T) * CHAR_BIT) - 1;
        static constexpr T kPendingZeroFlag = T(1) << (sizeof(T) * CHAR_BIT) - 2;

        mutable std::atomic<T> mCount;

    public:
        constexpr StickyCounter(T initial = 1) noexcept
            : mCount(initial)
        { }

        UTIL_NOCOPY(StickyCounter);

        T load(std::memory_order order = std::memory_order_seq_cst) const noexcept [[clang::reentrant, clang::nonblocking]] {
            T value = mCount.load(order);
            if (value == 0 && mCount.compare_exchange_strong(value, kZeroFlag | kPendingZeroFlag)) [[unlikely]] {
                return 0;
            }

            return (value & kZeroFlag) ? 0 : value;
        }

        /// @brief Increment the counter if it is not zero.
        ///
        /// @return If the counter is greater than zero.
        /// @retval true The counter was incremented and is greater than zero.
        /// @retval false The counter is stuck at zero.
        bool increment(T delta, std::memory_order order = std::memory_order_seq_cst) noexcept [[clang::reentrant, clang::nonblocking]] {
            T value = mCount.fetch_add(delta, order);
            return (value & kZeroFlag) == 0;
        }

        /// @brief Decrement the counter.
        ///
        /// @return If the counter was decremented to zero.
        /// @retval true The counter was decremented to zero by this operation.
        /// @retval false The counter is greater than zero or was already zero.
        bool decrement(T delta, std::memory_order order = std::memory_order_seq_cst) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (mCount.fetch_sub(delta, order) == delta) {
                T expected = 0;
                if (mCount.compare_exchange_strong(expected, kZeroFlag)) [[likely]] {
                    return true;
                } else if ((expected & kPendingZeroFlag) && (mCount.exchange(kZeroFlag) & kPendingZeroFlag)) {
                    return true;
                }
            }

            return false;
        }
    };
}
