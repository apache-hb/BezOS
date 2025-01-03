#pragma once

#include <concepts>

namespace sm {
    template<int First, int Last>
    struct Range {
        static constexpr int kFirst = First;
        static constexpr int kLast = Last;

        template<typename T, typename O>
        static constexpr T extract(O value) {
            T mask = (1ull << T(kLast - kFirst + 1)) - 1;
            return (value >> kFirst) & mask;
        }
    };

    template<typename T, auto...>
    class Field;

    template<typename T, Range... Ranges>
    class Field<T, Ranges...> {

    };

    template<int I>
    class Field<bool, I> {
    public:
        template<std::integral T>
        bool operator()(T value) const {
            return value & (1 << I);
        }
    };
}
