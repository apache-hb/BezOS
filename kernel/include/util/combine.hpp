#pragma once

#include "util/util.hpp"

#include <algorithm>

namespace sm {
    /// @brief Combine the storage of interface implementations into a single object.
    ///
    /// @warning This class is a bit of a footgun, if theres logic
    ///          in the copy or move constructors this won't invoke
    ///          that while copying/moving, causing lots of fun issues.
    template<typename T, typename... U> requires ((std::derived_from<U, T> && ...) && std::has_virtual_destructor_v<T>)
    class Combine {
        alignas(std::max(alignof(U)...)) char storage[std::max(sizeof(U)...)] {};

        constexpr T *pointer() { return reinterpret_cast<T *>(storage); }
        constexpr T **address() { return reinterpret_cast<T **>(&storage); }

        constexpr void destroy() {
            pointer()->~T();
        }

    public:
        constexpr Combine() = default;

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
