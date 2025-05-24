#pragma once

#include "std/detail/rcu_base.hpp"

#include "std/rcu.hpp"

namespace sm::detail {
    template<typename T>
    class CountedObject;

    template<typename T>
    class RetireSlots {
        using RetireCount = CounterInt;
        using ObjectPtr = CountedObject<T>*;

        class Slot final : public RcuObject {
            std::atomic<RetireCount> mCount{0};
            std::atomic<ObjectPtr> mObject{nullptr};

            RetireCount takeCounter() noexcept {
                return mCount.exchange(0);
            }

            ObjectPtr takeObject() noexcept {
                return mObject.exchange(nullptr);
            }

        public:
            /// @brief Prepare the slot for retiring an object.
            ///
            /// @return If the slot needs to be added to the retire list.
            /// @retval true The slot needs to be added to the retire list.
            /// @retval false The slot does not need to be added to the retire list.
            bool prepare(ObjectPtr object, RetireCount count) noexcept {
                if (RetireCount old = mCount.fetch_add(count); old > 0) {
                    return false;
                }

                mObject.store(object);
                return true;
            }

            static void ejectStrong(RcuDomain *domain, void *handle) noexcept {
                Slot *slot = static_cast<Slot*>(handle);
                ObjectPtr object = slot->takeObject();
                RetireCount count = slot->takeCounter();
                if (count == 0) {
                    return;
                }

                switch (object->releaseStrong(count)) {
                case EjectAction::eNone:
                    break;
                case EjectAction::eDestroy:
                    delete object;
                    break;
                case EjectAction::eDelay:
                    domain->retire(object);
                    break;
                }
            }

            static void ejectWeak(RcuDomain *domain [[maybe_unused]], void *handle) noexcept {
                Slot *slot = static_cast<Slot*>(handle);
                ObjectPtr object = slot->takeObject();
                RetireCount count = slot->takeCounter();
                if (count == 0) {
                    return;
                }

                if (object->releaseWeak(count)) {
                    delete object;
                }
            }
        };

        Slot mStrong;
        Slot mWeak;

    public:
        void retireStrong(RcuGuard& guard, CountedObject<T> *object, CounterInt count) noexcept {
            if (mStrong.prepare(object, count)) {
                guard.enqueue(&mStrong, Slot::ejectStrong);
            }
        }

        void retireWeak(RcuGuard& guard, CountedObject<T> *object, CounterInt count) noexcept {
            if (mWeak.prepare(object, count)) {
                guard.enqueue(&mWeak, Slot::ejectWeak);
            }
        }
    };
}
