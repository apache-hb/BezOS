#pragma once

#include <concepts>
#include <iterator>

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

namespace stdx {
    enum class Sign {
        SIGNED,
        UNSIGNED,
    };

    template<typename T, T V>
    struct Constant { static constexpr T kValue = V; };

    template<bool V>
    struct Bool : Constant<bool, V> { };
    struct False : Bool<false> { };
    struct True : Bool<true> { };

    template<typename T>
    struct IntegerSign { };

    template<Sign S>
    using SignConstant = Constant<Sign, S>;

    template<> struct IntegerSign<int8_t>  : SignConstant<Sign::SIGNED> {
        using Signed = int8_t;
        using Unsigned = uint8_t;
    };

    template<> struct IntegerSign<int16_t> : SignConstant<Sign::SIGNED> {
        using Signed = int16_t;
        using Unsigned = uint16_t;
    };

    template<> struct IntegerSign<int32_t> : SignConstant<Sign::SIGNED> {
        using Signed = int32_t;
        using Unsigned = uint32_t;
    };

    template<> struct IntegerSign<int64_t> : SignConstant<Sign::SIGNED> {
        using Signed = int64_t;
        using Unsigned = uint64_t;
    };

    template<> struct IntegerSign<uint8_t>  : SignConstant<Sign::UNSIGNED> {
        using Signed = int8_t;
        using Unsigned = uint8_t;
    };

    template<> struct IntegerSign<uint16_t> : SignConstant<Sign::UNSIGNED> {
        using Signed = int16_t;
        using Unsigned = uint16_t;
    };

    template<> struct IntegerSign<uint32_t> : SignConstant<Sign::UNSIGNED> {
        using Signed = int32_t;
        using Unsigned = uint32_t;
    };

    template<> struct IntegerSign<uint64_t> : SignConstant<Sign::UNSIGNED> {
        using Signed = int64_t;
        using Unsigned = uint64_t;
    };

    template<std::integral T>
    using Signed = typename IntegerSign<T>::Signed;

    template<std::integral T>
    using Unsigned = typename IntegerSign<T>::Unsigned;

    template<std::integral T>
    consteval bool isSigned(void) {
        return IntegerSign<T>::kValue == Sign::SIGNED;
    }

    template<std::integral T>
    consteval bool isUnsigned(void) {
        return IntegerSign<T>::kValue == Sign::UNSIGNED;
    }

    template<std::integral T>
    struct NumericTraits {
        static constexpr T kMin = isSigned<T>() ? T(1) << (sizeof(T) * CHAR_BIT - 1) : 0;
        static constexpr T kMax = isSigned<T>() ? ~kMin : ~T(0);
        static constexpr int kMaxDigits2 = sizeof(T) * CHAR_BIT;
        static constexpr int kMaxDigits8 = sizeof(T) * 3;
        static constexpr int kMaxDigits10 = sizeof(T) * 3;
        static constexpr int kMaxDigits16 = sizeof(T) * 2;
    };

    template<typename T, typename C>
    concept IsRange = requires(const C& range) {
        { std::begin(range) } -> std::convertible_to<T*>;
        { std::end(range) } -> std::convertible_to<T*>;
    };
}
