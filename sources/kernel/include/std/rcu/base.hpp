#pragma once

#include "std/rcu/weak.hpp"

namespace sm {
    namespace detail {
        class RcuIntrusiveBase { };
    }

    template<typename T>
    class RcuIntrusive : public detail::RcuIntrusiveBase {
        template<typename U>
        friend class RcuShared;

        template<typename U>
        friend class RcuAtomic;

        RcuWeak<T> mWeak;

        void setWeak(RcuWeak<T> weak) noexcept {
            mWeak = weak;
        }

    protected:
        RcuWeak<T> loanWeak() noexcept {
            return mWeak;
        }

        RcuShared<T> loanShared() noexcept {
            return mWeak.lock();
        }
    };

    template<typename T>
    concept HasIntrusiveRcuBase = std::derived_from<T, detail::RcuIntrusiveBase>;
}
