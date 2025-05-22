#pragma once

#include "std/rcu.hpp"

namespace sm {
    template<std::derived_from<RcuObject> T>
    class RcuProtect {
        T *mObject = nullptr;
        sm::RcuGuard mGuard;

    public:
        constexpr RcuProtect() noexcept = default;
        UTIL_NOCOPY(RcuProtect);

        RcuProtect(RcuDomain& domain, T *object) noexcept [[clang::reentrant, clang::nonblocking]]
            : mObject(object)
            , mGuard(domain)
        {
            KM_ASSERT(mObject != nullptr);
        }

        T *get() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mObject;
        }

        const T *get() noexcept [[clang::reentrant, clang::nonblocking]] {
            return mObject;
        }

        T *operator->() noexcept [[clang::reentrant, clang::nonblocking]] {
            return get();
        }

        const T *operator->() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return get();
        }
    };
}
