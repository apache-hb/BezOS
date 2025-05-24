#pragma once

#include <memory>
#include <new>

#include <stdint.h>

#include "std/detail/retire_slots.hpp"
#include "std/detail/sticky_counter.hpp"
#include "std/rcu.hpp"

namespace sm::detail {
    template<typename T>
    class CountedObject : public sm::RcuObject {
        alignas(alignof(T)) char mStorage[sizeof(T)];
        StickyCounter<CounterInt> mStrongCount;
        StickyCounter<CounterInt> mWeakCount;
        RetireSlots<T> mSlots;
        sm::RcuDomain *mDomain;

        friend class RetireSlots<T>;

        void destroy() noexcept {
            std::destroy_at(&get());
        }

    public:
        constexpr CountedObject(sm::RcuDomain *domain) noexcept
            : mStrongCount(1)
            , mWeakCount(1)
            , mDomain(domain)
        {
            new (mStorage) T();
        }

        template<typename... Args>
        CountedObject(sm::RcuDomain *domain, Args&&... args) noexcept
            : mStrongCount(1)
            , mWeakCount(1)
            , mDomain(domain)
        {
            new (mStorage) T(std::forward<Args>(args)...);
        }

        T& get() noexcept {
            return *std::launder(reinterpret_cast<T*>(mStorage));
        }

        const T& get() const noexcept {
            return *std::launder(reinterpret_cast<const T*>(mStorage));
        }

        CounterInt strongCount() const noexcept {
            return mStrongCount.load();
        }

        CounterInt weakCount() const noexcept {
            return mWeakCount.load();
        }

        EjectAction releaseStrong(CounterInt count) noexcept {
            if (mStrongCount.decrement(count)) {
                if (mWeakCount.load() == 1) {
                    destroy();
                    return EjectAction::eDestroy;
                } else {
                    return EjectAction::eDelay;
                }
            }

            return EjectAction::eNone;
        }

        bool releaseWeak(CounterInt count) noexcept {
            return mWeakCount.decrement(count);
        }

        bool retainStrong(CounterInt count) noexcept {
            return mStrongCount.increment(count);
        }

        bool retainWeak(CounterInt count) noexcept {
            return mWeakCount.increment(count);
        }

        void deferReleaseStrong(RcuGuard& guard, CounterInt count) noexcept {
            mSlots.retireStrong(guard, this, count);
        }

        void deferReleaseWeak(RcuGuard& guard, CounterInt count) noexcept {
            mSlots.retireWeak(guard, this, count);
        }
    };
}
