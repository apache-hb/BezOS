#pragma once

#include <concepts>

namespace km {
    template<std::integral T>
    constexpr T roundpow2(T value, T multiple) {
        return (value + multiple - 1) & ~(multiple - 1);
    }

    template<std::integral T>
    constexpr T roundup(T value, T multiple) {
        return (value + multiple - 1) / multiple * multiple;
    }
}
