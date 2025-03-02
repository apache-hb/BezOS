#pragma once

#include <concepts>
#include <cstddef>
#include <utility> // IWYU pragma: keep - std::to_underlying

namespace sm {
    struct init {};
    struct noinit {};

    template<std::integral T>
    constexpr T roundpow2(T value, T multiple) {
        return (value + multiple - 1) & ~(multiple - 1);
    }

    template<std::integral T>
    constexpr T roundup(T value, T multiple) {
        return (value + multiple - 1) / multiple * multiple;
    }

    template<std::integral T>
    constexpr T rounddown(T value, T multiple) {
        return value / multiple * multiple;
    }

    constexpr bool isPowerOf2(std::integral auto value) {
        return value && !(value & (value - 1));
    }

    template<std::integral T>
    constexpr T nextPowerOf2(T value) {
        value--;
        for (size_t i = 1; i < sizeof(T) * 8; i *= 2) {
            value |= value >> i;
        }
        return value + 1;
    }

    template<typename T>
    constexpr T nextMultiple(T value, T multiple) {
        if (value % multiple == 0) return value + multiple;
        return (value + multiple - 1) / multiple * multiple;
    }

    template<std::integral T>
    constexpr T magnitude(T value) {
        return value < 0 ? -value : value;
    }

    template<typename T>
    struct AlwaysFalse {
        static constexpr bool Value = false;
    };
}

#define UTIL_BITFLAGS(it) \
    inline constexpr it operator|(it lhs, it rhs) { return it(std::to_underlying(lhs) | std::to_underlying(rhs)); } \
    inline constexpr it operator&(it lhs, it rhs) { return it(std::to_underlying(lhs) & std::to_underlying(rhs)); } \
    inline constexpr it operator^(it lhs, it rhs) { return it(std::to_underlying(lhs) ^ std::to_underlying(rhs)); } \
    inline constexpr it operator~(it rhs) { return it(~std::to_underlying(rhs)); } \
    inline it& operator|=(it& lhs, it rhs) { return lhs = lhs | rhs; } \
    inline it& operator&=(it& lhs, it rhs) { return lhs = lhs & rhs; } \
    inline it& operator^=(it& lhs, it rhs) { return lhs = lhs ^ rhs; }

#define UTIL_NOCOPY(it) \
    it(const it&) = delete; \
    it& operator=(const it&) = delete;

#define UTIL_NOMOVE(it) \
    it(it&&) = delete; \
    it& operator=(it&&) = delete;

#ifdef UTIL_TESTING
#   define WEAK_SYMBOL_TEST [[gnu::weak]]
#else
#   define WEAK_SYMBOL_TEST
#endif
