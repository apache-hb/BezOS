#pragma once

#include <bezos/status.h>
#include <stddef.h>

#include "common/util/util.hpp"

namespace sm {
    /// @brief A value or pointer that is aligned to N bytes.
    ///
    /// @tparam T The type of the value or pointer.
    /// @tparam N The alignment in bytes.
    template<typename T, size_t N>
    class AlignedValue {
        static_assert(sm::isPowerOf2(N), "Alignment must be a power of 2");

        T mValue{};

    public:
        static constexpr size_t alignment() noexcept {
            return N;
        }

        constexpr operator T() const noexcept [[clang::nonblocking]] {
            return mValue;
        }

        constexpr T get() const noexcept [[clang::nonblocking]] {
            return mValue;
        }

        constexpr const T *operator&() const noexcept [[clang::nonblocking]] {
            return &mValue;
        }

        [[nodiscard]]
        constexpr OsStatus set(T value) noexcept [[clang::nonblocking]] {
            if (((uintptr_t)value % alignment()) != 0) {
                return OsStatusInvalidInput;
            }

            mValue = value;
            return OsStatusSuccess;
        }

        [[nodiscard]]
        static OsStatus create(T value, AlignedValue *result) noexcept [[clang::nonallocating]] {
            return result->set(value);
        }
    };
}