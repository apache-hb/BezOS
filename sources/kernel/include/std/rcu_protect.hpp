#pragma once

#include "std/detail/sticky_counter.hpp"
#include "std/rcu.hpp"

namespace sm {
    class RcuProtect;

    template<std::derived_from<RcuProtect> T>
    bool rcuRetire(RcuGuard& guard, T *object) noexcept [[clang::reentrant, clang::nonblocking]];

    template<std::derived_from<RcuProtect> T>
    bool rcuRetain(T *object) noexcept [[clang::reentrant, clang::nonblocking]];

    class RcuProtect : public RcuObject {
        detail::WaitFreeCounter<uint32_t> mOwners;

        template<std::derived_from<RcuProtect> T>
        friend bool rcuRetire(RcuGuard& guard, T *object) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (object->mOwners.decrement(1)) {
                guard.retire(object);
                return true;
            }

            return false;
        }

        template<std::derived_from<RcuProtect> T>
        friend bool rcuRetain(T *object) noexcept [[clang::reentrant, clang::nonblocking]] {
            return object->mOwners.increment(1);
        }
    };
}
