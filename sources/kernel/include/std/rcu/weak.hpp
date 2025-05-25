#pragma once

#include "std/detail/counted.hpp"
#include "std/rcu/shared.hpp"

namespace sm {
    template<typename T>
    class RcuWeak {
        using Counted = sm::detail::CountedObject<T>;
        Counted *mControl { nullptr };

        template<typename U>
        friend class RcuAtomic;

        Counted *exchangeControl(Counted *other = nullptr) noexcept {
            return std::exchange(mControl, other);
        }

        RcuWeak(Counted *control) noexcept
            : mControl(control)
        { }

    public:
        constexpr RcuWeak() noexcept = default;

        RcuWeak(const RcuWeak& other) noexcept
            : mControl(other.mControl)
        {
            if (mControl) {
                mControl->retainWeak(1);
            }
        }

        RcuWeak(RcuWeak&& other) noexcept
            : mControl(std::exchange(other.mControl, nullptr))
        { }

        RcuWeak& operator=(const RcuWeak& other) noexcept {
            if (this != &other) {
                Counted *old = std::exchange(mControl, other.mControl);
                if (mControl) {
                    mControl->retainWeak(1);
                }

                if (old) {
                    old->deferReleaseWeak(1);
                }
            }
            return *this;
        }

        RcuWeak& operator=(RcuWeak&& other) noexcept {
            if (this != &other) {
                Counted *old = std::exchange(mControl, std::exchange(other.mControl, nullptr));
                if (old) {
                    old->deferReleaseWeak(1);
                }
            }
            return *this;
        }

        ~RcuWeak() noexcept {
            reset();
        }

        void reset() noexcept {
            if (Counted *object = exchangeControl()) {
                object->deferReleaseWeak(1);
            }
        }

        RcuShared<T> lock() noexcept {
            if (mControl && mControl->retainStrong(1)) {
                return RcuShared<T>(mControl);
            }
            return nullptr;
        }
    };
}
