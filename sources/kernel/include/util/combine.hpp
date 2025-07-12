#pragma once

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace sm {
    /// @brief Combine the storage of interface implementations into a single object.
    ///
    /// @warning This class is a bit of a footgun, if theres logic
    ///          in the copy or move constructors this won't invoke
    ///          that while copying/moving, causing lots of fun issues.
    template<typename T, typename... U> requires ((std::derived_from<U, T> && ...) && std::has_virtual_destructor_v<T>)
    class Combine {
        alignas(std::max(alignof(U)...)) char mStorage[std::max(sizeof(U)...)] {};

        /// @brief Checks if the vtable pointer is valid.
        ///
        /// @note This is valid only under the itanium x64 abi, and only
        ///       for classes with single inheritance.
        bool isValid() const {
            char zero[sizeof(void*)]{};
            return std::memcmp(mStorage, zero, sizeof(void*)) != 0;
        }

        constexpr void destroy() {
            if (isValid()) {
                pointer()->~T();
                std::memset(mStorage, 0, sizeof(void*));
            }
        }

    public:
        constexpr Combine() = default;

        template<typename O> requires (std::same_as<std::remove_reference_t<O>, U> || ...)
        constexpr Combine(O &&o) {
            new (address()) std::remove_reference_t<O>{std::forward<O>(o)};
        }

        constexpr ~Combine() {
            destroy();
        }

        template<typename O> requires (std::same_as<std::remove_reference_t<O>, U> || ...)
        constexpr Combine &operator=(O &&o) {
            destroy();
            new (address()) std::remove_reference_t<O>{std::forward<O>(o)};
            return *this;
        }

        T *pointer() noexcept [[clang::reentrant]] { return reinterpret_cast<T *>(mStorage); }
        T **address() noexcept [[clang::reentrant]] { return reinterpret_cast<T **>(&mStorage); }

        T *operator*() noexcept [[clang::reentrant]] { return pointer(); }
        T *operator->() noexcept [[clang::reentrant]] { return pointer(); }
        T **operator&() noexcept [[clang::reentrant]] { return address(); }
    };
}
