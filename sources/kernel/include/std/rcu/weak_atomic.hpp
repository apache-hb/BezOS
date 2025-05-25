#pragma once

#include "std/detail/counted.hpp"
#include "std/rcu/weak.hpp"

namespace sm {
    template<typename T>
    class RcuWeakAtomic {
        using Counted = sm::detail::CountedObject<T>;
        std::atomic<Counted*> mControl { nullptr };

    public:
        constexpr RcuWeakAtomic() noexcept = default;

        UTIL_NOCOPY(RcuWeakAtomic);
        UTIL_NOMOVE(RcuWeakAtomic);

        constexpr RcuWeakAtomic(std::nullptr_t) noexcept
            : mControl(nullptr)
        { }

        constexpr RcuWeakAtomic(RcuWeak<T> object) noexcept
            : mControl(object.exchangeControl())
        { }

        ~RcuWeakAtomic() noexcept {
            reset();
        }

        void store(RcuWeak<T> object, std::memory_order order = std::memory_order_seq_cst) noexcept {
            Counted *newObject = object.exchangeControl();
            if (Counted *old = mControl.exchange(newObject, order)) {
                old->deferReleaseWeak(1);
            }
        }

        RcuWeak<T> load(std::memory_order order = std::memory_order_seq_cst) noexcept {
            Counted *object = mControl.load(order);
            if (object && object->retainWeak(1)) {
                return RcuWeak<T>(object);
            }
            return nullptr;
        }

        void reset() noexcept {
            if (Counted *object = mControl.exchange(nullptr)) {
                object->deferReleaseWeak(1);
            }
        }
    };
}
