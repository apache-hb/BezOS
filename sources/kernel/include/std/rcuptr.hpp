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
            constexpr AtomicStickyZero() = default;
            constexpr AtomicStickyZero(T initial)
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

            /// @brief Peek at the current value of the counter.
            ///
            /// For debugging purposes only.
            T peek() {
                return mCount.load();
            }
        };

        struct ControlBlock {
            AtomicStickyZero<uint32_t> strong = 1;
            AtomicStickyZero<uint32_t> weak = 1;
            void *value = nullptr;
            void(*deleter)(void*) = nullptr;
        };

        void RcuReleaseStrong(void *ptr);
        bool RcuAcqiureStrong(ControlBlock& control);
        void RcuReleaseWeak(void *ptr);
        bool RcuAcquireWeak(ControlBlock& control);
    }

    template<typename T>
    class RcuIntrusivePtr;

    template<typename T>
    class RcuSharedPtr;

    template<typename T>
    class RcuWeakPtr;

    template<typename T>
    concept IsIntrusivePtr = std::derived_from<T, RcuIntrusivePtr<T>>;

    template<typename T>
    class RcuSharedPtr {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>>, "Cannot use RcuSharedPtr with reference types");
        static_assert(!std::is_same_v<T, rcu::detail::ControlBlock>, "Cannot use RcuSharedPtr with ControlBlock");

        template<typename O>
        friend class RcuWeakPtr;

        RcuDomain *mDomain;
        rcu::detail::ControlBlock *mControl;

        void release() {
            if (mControl != nullptr) {
                rcu::detail::RcuReleaseStrong(mControl);
            }

            mDomain = nullptr;
            mControl = nullptr;
        }

        void acquire(RcuDomain *domain, rcu::detail::ControlBlock *control) {
            if (control != nullptr) {
                if (!rcu::detail::RcuAcqiureStrong(*control)) {
                    return;
                }
            }

            mDomain = domain;
            mControl = control;
        }

        constexpr RcuSharedPtr(RcuDomain *domain, rcu::detail::ControlBlock *control) : RcuSharedPtr() {
            acquire(domain, control);
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

        constexpr RcuSharedPtr(RcuDomain *domain, T *ptr);

        constexpr RcuSharedPtr(const RcuSharedPtr& other) : RcuSharedPtr() {
            if (other.mDomain) {
                acquire(other.mDomain, other.mControl);
            }
        }

        constexpr RcuSharedPtr& operator=(const RcuSharedPtr& other) {
            release();

            if (other.mDomain) {
                acquire(other.mDomain, other.mControl);
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
            release();

            if (other.mDomain) {
                mDomain = std::exchange(other.mDomain, nullptr);
                mControl = std::exchange(other.mControl, nullptr);
            }

            return *this;
        }

        RcuWeakPtr<T> weak();

        constexpr T *operator->() {
            return static_cast<T*>(mControl->value);
        }

        constexpr T *get() {
            return static_cast<T*>(mControl->value);
        }

        constexpr T& operator*(this auto&& self) {
            return *static_cast<T*>(self.mControl->value);
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
            if (control != nullptr) {
                if (!rcu::detail::RcuAcquireWeak(*control)) {
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

        constexpr RcuWeakPtr(std::nullptr_t)
            : mDomain(nullptr)
            , mControl(nullptr)
        { }

        constexpr ~RcuWeakPtr() {
            release();
        }

        constexpr RcuWeakPtr(const RcuSharedPtr<T>& shared) : RcuWeakPtr() {
            acquire(shared.mDomain, shared.mControl);
        }

        constexpr RcuWeakPtr(const RcuWeakPtr& other) : RcuWeakPtr() {
            acquire(other.mDomain, other.mControl);
        }

        constexpr RcuWeakPtr& operator=(const RcuWeakPtr& other) {
            release();

            acquire(other.mDomain, other.mControl);

            return *this;
        }

        constexpr RcuWeakPtr(RcuWeakPtr&& other) : RcuWeakPtr() {
            std::swap(mDomain, other.mDomain);
            std::swap(mControl, other.mControl);
        }

        constexpr RcuWeakPtr& operator=(RcuWeakPtr&& other) {
            release();

            mDomain = std::exchange(other.mDomain, nullptr);
            mControl = std::exchange(other.mControl, nullptr);

            return *this;
        }

        RcuSharedPtr<T> lock() {
            return RcuSharedPtr<T>(mDomain, mControl);
        }
    };

    template<typename T>
    class RcuIntrusivePtr {
        friend class RcuSharedPtr<T>;
        friend class RcuWeakPtr<T>;

        RcuWeakPtr<T> mWeakThis;

        void initWeak(RcuWeakPtr<T> weak) {
            mWeakThis = weak;
        }

    public:
        RcuWeakPtr<T> loanWeak() {
            return mWeakThis;
        }

        RcuSharedPtr<T> loanShared() {
            return mWeakThis.lock();
        }
    };

    template<typename T, typename... Args>
    RcuSharedPtr<T> rcuMakeShared(RcuDomain *domain, Args&&... args) {
        if (T *ptr = new (std::nothrow) T(std::forward<Args>(args)...)) {
            if (RcuSharedPtr rcu = RcuSharedPtr<T>{domain, ptr}) {
                return rcu;
            }

            delete ptr;
        }

        return nullptr;
    }

    template<typename T>
    constexpr RcuSharedPtr<T>::RcuSharedPtr(RcuDomain *domain, T *ptr)
        : mDomain(domain)
        , mControl(new (std::nothrow) rcu::detail::ControlBlock { 1, 1, ptr, [](void *ptr) { delete static_cast<T*>(ptr); } })
    {
        if (mControl != nullptr && ptr != nullptr) {
            if constexpr (IsIntrusivePtr<T>) {
                ptr->initWeak(weak());
            }
        }
    }

    template<typename T>
    RcuWeakPtr<T> RcuSharedPtr<T>::weak() {
        return RcuWeakPtr<T>(*this);
    }
}

template<typename T>
struct std::hash<sm::RcuSharedPtr<T>> {
    constexpr size_t operator()(const sm::RcuSharedPtr<T>& ptr) const noexcept {
        return std::hash<T*>{}(ptr.get());
    }
};
