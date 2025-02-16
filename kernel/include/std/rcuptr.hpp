#pragma once

#include "std/rcu.hpp"

#include <climits>

namespace sm {
    namespace rcu::detail {
        template<typename T>
        class AtomicStickyZero {
            static constexpr T kIsZero = (T(1) << (sizeof(T) * CHAR_BIT - 1));
            std::atomic<T> mCount;

        public:
            AtomicStickyZero() = default;
            AtomicStickyZero(T initial)
                : mCount(initial)
            { }

            /// @brief Increment the counter if it isn't zero.
            ///
            /// @return False if the counter is stuck to zero, true otherwise.
            bool increment() {
                return (mCount.fetch_add(1) & kIsZero) == 0;
            }

            /// @brief Decrement the counter.
            ///
            /// @return True if the counter is now zero, false otherwise.
            bool decrement() {
                if (mCount.fetch_sub(1) == 1) {
                    T expected = 0;
                    return mCount.compare_exchange_strong(expected, kIsZero);
                }

                return false;
            }
        };

        struct ControlBlock {
            AtomicStickyZero<uint32_t> strong;
            AtomicStickyZero<uint32_t> weak;
            void *value;
            void(*deleter)(void*);
        };

        void RcuReleaseStrong(void *ptr);
        bool RcuAcqiureStrong(ControlBlock& control);
        void RcuReleaseWeak(void *ptr);
        bool RcuAcquireWeak(ControlBlock& control);
    }

    template<typename T>
    class RcuSharedPtr;

    template<typename T>
    class RcuWeakPtr;

    template<typename T>
    class RcuSharedPtr {
        template<typename O>
        friend class RcuWeakPtr;

        RcuDomain *mDomain;
        rcu::detail::ControlBlock *mControl;

        void release() {
            if (mDomain != nullptr && mControl != nullptr) {
                mDomain->call(mControl, rcu::detail::RcuReleaseStrong);
            }

            mDomain = nullptr;
            mControl = nullptr;
        }

        void acquire(RcuDomain *domain, rcu::detail::ControlBlock *control) {
            if (domain != nullptr && control != nullptr) {
                if (!RcuAcqiureStrong(*control)) {
                    return;
                }
            }

            mDomain = domain;
            mControl = control;
        }

    public:
        constexpr RcuSharedPtr()
            : mDomain(nullptr)
            , mControl(nullptr)
        { }

        constexpr RcuSharedPtr(std::nullptr_t)
            : mDomain(nullptr)
            , mControl(nullptr)
        { }

        constexpr ~RcuSharedPtr() {
            release();
        }

        constexpr RcuSharedPtr(RcuDomain *domain, T *ptr)
            : mDomain(domain)
            , mControl(new rcu::detail::ControlBlock { 1, 1, ptr, [](void *ptr) { delete static_cast<T*>(ptr); } })
        { }

        constexpr RcuSharedPtr(const RcuSharedPtr& other) : RcuSharedPtr() {
            if (other.mDomain) {
                acquire(other.mDomain, other.mControl);
            }
        }

        constexpr RcuSharedPtr& operator=(const RcuSharedPtr& other) {
            if (other.mDomain) {
                release();
                acquire(other.mDomain, other.mControl);
            } else {
                release();
            }

            return *this;
        }

        constexpr RcuSharedPtr(RcuSharedPtr&& other) : RcuSharedPtr() {
            if (other.mDomain) {
                mDomain = std::exchange(other.mDomain, nullptr);
                mControl = std::exchange(other.mControl, nullptr);
            }
        }

        constexpr RcuSharedPtr& operator=(RcuSharedPtr&& other) {
            if (other.mDomain) {
                release();
                mDomain = std::exchange(other.mDomain, nullptr);
                mControl = std::exchange(other.mControl, nullptr);
            } else {
                release();
            }

            return *this;
        }

        constexpr T *operator->() {
            return static_cast<T*>(mControl->value);
        }

        constexpr T *get() {
            return static_cast<T*>(mControl->value);
        }

        constexpr operator bool() const {
            return mControl != nullptr;
        }

        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuSharedPtr<O>& other) const {
            return mControl == other.mControl;
        }

        constexpr bool operator==(std::nullptr_t) const {
            return mControl == nullptr;
        }
    };

    template<typename T>
    class RcuWeakPtr {
        RcuDomain *mDomain;
        rcu::detail::ControlBlock *mControl;

        void release() {
            if (mDomain != nullptr && mControl != nullptr) {
                mDomain->call(mControl, rcu::detail::RcuReleaseWeak);
            }
        }

        void acquire(RcuDomain *domain, rcu::detail::ControlBlock *control) {
            if (domain != nullptr && control != nullptr) {
                if (!RcuAcquireWeak(*control)) {
                    return;
                }
            }

            mDomain = domain;
            mControl = control;
        }

    public:
        constexpr RcuWeakPtr()
            : mDomain(nullptr)
            , mControl(nullptr)
        { }

        constexpr ~RcuWeakPtr() {
            release();
        }

        constexpr RcuWeakPtr(const RcuSharedPtr<T>& shared) : RcuWeakPtr() {
            RcuGuard guard(*shared.mDomain);
            acquire(shared.mDomain, shared.mControl);
        }

        constexpr RcuWeakPtr(const RcuWeakPtr& other) : RcuWeakPtr() {
            acquire(other.mDomain, other.mControl);
        }

        constexpr RcuWeakPtr& operator=(const RcuWeakPtr& other) {
            RcuGuard guard(*other.mDomain);
            release();
            acquire(other.mDomain, other.mControl);
        }

        constexpr RcuWeakPtr(RcuWeakPtr&& other) : RcuWeakPtr() {
            std::swap(mDomain, other.mDomain);
            std::swap(mControl, other.mControl);
        }

        constexpr RcuWeakPtr& operator=(RcuWeakPtr&& other) {
            RcuGuard guard(*other.mDomain);
            release();
            mDomain = std::exchange(other.mDomain, nullptr);
            mControl = std::exchange(other.mControl, nullptr);
        }

        RcuSharedPtr<T> lock() {
            return RcuSharedPtr<T>(mDomain, mControl);
        }
    };
}
