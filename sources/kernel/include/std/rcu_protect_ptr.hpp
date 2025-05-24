#pragma once

#include "std/detail/counted.hpp"

namespace sm {
    class RcuDomain;

    template<typename T>
    class RcuProtectPtr {
        using CountedPtr = sm::detail::CountedObject<T>*;
        std::atomic<CountedPtr> mControl;
        sm::RcuDomain *mDomain;

    public:
        RcuProtectPtr() noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        RcuProtectPtr(std::nullptr_t) noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        ~RcuProtectPtr() noexcept [[clang::reentrant, clang::nonblocking]] {
            if (auto control = mControl.load()) {

            }
        }

        UTIL_NOCOPY(RcuProtectPtr);
        UTIL_NOMOVE(RcuProtectPtr);
    };
}
