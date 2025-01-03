#pragma once

namespace sm {
    template<int First, int Last>
    struct Range {
        static constexpr int kFirst = First;
        static constexpr int kLast = Last;
    };

    template<typename T, auto...>
    class Field;

    template<typename T, Range... Ranges>
    class Field<T, Ranges...> {

    };

    template<int I>
    class Field<bool, I> {

    };
}
