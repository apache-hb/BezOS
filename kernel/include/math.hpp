#pragma once

#include <stddef.h>

#include <tuple>

namespace sm::math {
    template<typename T>
    struct Vec2 {
        union {
            T fields[2];
            struct { T x; T y; };
        };

        constexpr Vec2(T x, T y)
            : x(x)
            , y(y)
        { }

        template<size_t N>
        constexpr decltype(auto) get() const {
            if constexpr (N == 0) return x;
            else if constexpr (N == 1) return y;
            else static_assert(N < 2, "Index out of bounds");
        }
    };

    using int2 = Vec2<int>;
}

// Overloads for structured binding support

template<typename T>
struct std::tuple_size<sm::math::Vec2<T>> : std::integral_constant<size_t, 2> { };

template<size_t I, typename T>
struct std::tuple_element<I, sm::math::Vec2<T>> { using type = T; };
