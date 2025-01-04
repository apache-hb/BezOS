#pragma once

#include "util/util.hpp"

#include <algorithm>

namespace sm {
    template<typename T, typename... U> requires ((std::derived_from<U, T> && ...) && std::has_virtual_destructor_v<T>)
    class Combine {
        alignas(std::max(alignof(U)...)) char storage[std::max(sizeof(U)...)];

        constexpr T *pointer() { return reinterpret_cast<T *>(storage); }
        constexpr T **address() { return reinterpret_cast<T **>(&storage); }

        constexpr void destroy() {
            pointer()->~T();
        }

    public:
        UTIL_NOCOPY(Combine);
        UTIL_NOMOVE(Combine);

        template<typename O> requires (std::same_as<O, U> || ...)
        constexpr Combine(O &&o) {
            new (address()) O{std::forward<O>(o)};
        }

        constexpr ~Combine() {
            destroy();
        }

        template<typename O> requires (std::same_as<O, U> || ...)
        constexpr Combine &operator=(O &&o) {
            destroy();
            new (address()) O{std::forward<O>(o)};
            return *this;
        }

        T *operator->() { return pointer(); }
        T **operator&() { return address(); }
    };
}
