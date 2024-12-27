#pragma once

#include <concepts>
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
}

#define UTIL_BITFLAGS(it) \
    inline constexpr it operator|(it lhs, it rhs) { return it(std::to_underlying(lhs) | std::to_underlying(rhs)); } \
    inline constexpr it operator&(it lhs, it rhs) { return it(std::to_underlying(lhs) & std::to_underlying(rhs)); } \
    inline constexpr it operator^(it lhs, it rhs) { return it(std::to_underlying(lhs) ^ std::to_underlying(rhs)); } \
    inline constexpr it operator~(it rhs) { return it(~std::to_underlying(rhs)); } \
    inline it& operator|=(it& lhs, it rhs) { return lhs = lhs | rhs; } \
    inline it& operator&=(it& lhs, it rhs) { return lhs = lhs & rhs; } \
    inline it& operator^=(it& lhs, it rhs) { return lhs = lhs ^ rhs; }
