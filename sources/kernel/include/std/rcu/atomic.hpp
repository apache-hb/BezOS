#pragma once

#include "std/detail/counted.hpp"
#include "std/rcu/shared.hpp"

namespace sm {
    template<typename T>
    class RcuAtomic {
        using Counted = sm::detail::CountedObject<T>;
        std::atomic<Counted*> mControl { nullptr };

    public:
        constexpr RcuAtomic() noexcept = default;
        UTIL_NOCOPY(RcuAtomic);
        UTIL_NOMOVE(RcuAtomic);

        constexpr RcuAtomic(std::nullptr_t) noexcept
            : mControl(nullptr)
        { }

        template<typename... Args>
        RcuAtomic(sm::RcuDomain *domain, Args&&... args) noexcept
            : mControl(new (std::nothrow) Counted(domain, std::forward<Args>(args)...))
        { }

        RcuAtomic(sm::RcuDomain *domain) noexcept
            : mControl(new (std::nothrow) Counted(domain))
        { }

        ~RcuAtomic() noexcept {
            reset();
        }

        void store(RcuShared<T> object, std::memory_order order = std::memory_order_seq_cst) noexcept {
            Counted *newObject = object.exchangeControl();
            if (Counted *old = mControl.exchange(newObject, order)) {
                old->deferReleaseStrong(1);
            }
        }

        RcuShared<T> load(std::memory_order order = std::memory_order_seq_cst) noexcept {
            Counted *object = mControl.load(order);
            if (object && object->retainStrong(1)) {
                return RcuShared<T>(object);
            }
            return nullptr;
        }

        bool compare_exchange_weak(RcuShared<T>& expected, const RcuShared<T>& desired) noexcept {
            Counted *expectedObject = expected.mControl;
            Counted *desiredObject = desired.mControl;
            if (!mControl.compare_exchange_strong(expectedObject, desiredObject)) {
                expected = load();
                return false;
            } else {
                if (desiredObject) {
                    desiredObject->retainStrong(1);
                }

                if (expectedObject) {
                    expectedObject->deferReleaseStrong(1);
                }
                return true;
            }
        }

        void reset() noexcept {
            if (Counted *object = mControl.exchange(nullptr)) {
                object->deferReleaseStrong(1);
            }
        }

        OsStatus reset(sm::RcuDomain *domain) noexcept {
            Counted *newObject = new (std::nothrow) Counted(domain);
            if (!newObject) {
                return OsStatusOutOfMemory;
            }

            Counted *oldObject = mControl.exchange(newObject);
            if (oldObject) {
                oldObject->deferReleaseStrong(1);
            }

            return OsStatusSuccess;
        }

        template<typename... Args>
        OsStatus reset(sm::RcuDomain *domain, Args&&... args) noexcept {
            Counted *newObject = new (std::nothrow) Counted(domain, std::forward<Args>(args)...);
            if (!newObject) {
                return OsStatusOutOfMemory;
            }

            Counted *oldObject = mControl.exchange(newObject);
            if (oldObject) {
                oldObject->deferReleaseStrong(1);
            }

            return OsStatusSuccess;
        }
    };
}
