#pragma once

#include <algorithm>
#include <array>

namespace util {
    template<std::size_t N>
    constexpr std::array<char, N - 1> Signature(const char (&text)[N]) {
        std::array<char, N - 1> result;
        std::copy(text, text + N - 1, result.begin());
        return result;
    }
}
