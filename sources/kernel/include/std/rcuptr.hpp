#pragma once

#include "std/rcu.hpp"

#include <climits>

namespace sm {
    namespace rcu::detail {
        class JointCount {
            static constexpr uint64_t kStrongIsZero = (UINT64_C(1) << 63);
            static constexpr uint64_t kWeakIsZero = (UINT64_C(1) << 31);
            static constexpr uint64_t kStrongMask = 0xFFFFFFFF00000000;
            static constexpr uint64_t kWeakMask = 0x00000000FFFFFFFF;
            static constexpr uint64_t kStrongOne = (UINT64_C(1) << 32);
            static constexpr uint64_t kWeakOne = (UINT64_C(1) << 0);

            /// @brief The joint reference count
            /// The high 32 bits are the strong reference count.
            /// The low 32 bits are the weak reference count.
            std::atomic<uint64_t> mCount;

        public:
            enum Release { eNone = 0, eStrong = (1 << 0), eWeak = (1 << 1) };

            constexpr JointCount() = default;
            constexpr JointCount(uint32_t strong, uint32_t weak)
                : mCount((uint64_t(strong) << 32) | uint64_t(weak))
            { }

            /// @brief Increment the weak reference count.
            /// @return False if the strong reference count is zero, true otherwise.
            bool weakRetain() {
                return (mCount.fetch_add(kWeakOne) & kWeakIsZero) == 0;
            }

            /// @brief Decrement the weak reference count.
            /// @return True if the weak count was brought to zero by this operation, false otherwise.
            bool weakRelease() {
                if (uint64_t count = mCount.fetch_sub(kWeakOne); (count & kWeakMask) == kWeakOne) {
                    uint64_t expected = count & ~kWeakMask;
                    return mCount.compare_exchange_strong(expected, expected | kWeakIsZero);
                }

                return false;
            }

            /// @brief Increment the weak and strong reference count.
            /// @return False if the strong reference count is zero, true otherwise.
            bool strongRetain() {
                if ((mCount.fetch_add(kStrongOne) & kStrongIsZero) == 0) {
                    weakRetain();
                    return true;
                }

                return false;
            }

            /// @brief Decrement the weak and strong reference count.
            /// @return A mask of which reference counts were brought to zero by this operation.
            Release strongRelease() {
                bool weakCleared = false;
                bool strongCleared = false;
                if (uint64_t count = mCount.fetch_sub(kStrongOne | kWeakOne); ((count & kStrongMask) == kStrongOne || (count & kWeakMask) == kWeakOne)) {
                    weakCleared = (count & kWeakMask) == kWeakOne;
                    strongCleared = (count & kStrongMask) == kStrongOne;

                    uint64_t expected = count;
                    uint64_t value = expected;
                    if (weakCleared) {
                        expected = (expected & ~kWeakMask) | kWeakIsZero;
                        value |= kWeakIsZero;
                    }

                    if (strongCleared) {
                        expected = (expected & ~kStrongMask) | kStrongIsZero;
                        value |= kStrongIsZero;
                    }

                    while (!mCount.compare_exchange_strong(expected, value)) {
                        value = expected;

                        weakCleared = (expected & kWeakMask) == 0;
                        strongCleared = (expected & kStrongMask) == 0;

                        if (weakCleared) {
                            expected = (expected & ~kWeakMask);
                            value |= kWeakIsZero;
                        }

                        if (strongCleared) {
                            expected = (expected & ~kStrongMask);
                            value |= kStrongIsZero;
                        }
                    }
                }

                int result = eNone;
                if (strongCleared)
                    result |= eStrong;
                if (weakCleared)
                    result |= eWeak;

                return Release(result);
            }
        };

        UTIL_BITFLAGS(JointCount::Release);

        struct ControlBlock {
            JointCount count = { 1, 1 };
            void *value = nullptr;
            void(*deleter)(void*) = nullptr;
            RcuDomain *domain = nullptr;
        };

        void RcuReleaseStrong(ControlBlock *ptr);
        bool RcuAcqiureStrong(ControlBlock *control);
        void RcuReleaseWeak(ControlBlock *cb);
        bool RcuAcquireWeak(ControlBlock *control);
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

        std::atomic<rcu::detail::ControlBlock*> mControl;

        void exchangeControl(rcu::detail::ControlBlock *control) {
            if (rcu::detail::ControlBlock *cb = mControl.exchange(control)) {
                rcu::detail::RcuReleaseStrong(cb);
            }
        }

        void release() {
            exchangeControl(nullptr);
        }

        void acquire(rcu::detail::ControlBlock *control) {
            if (control != nullptr && rcu::detail::RcuAcqiureStrong(control)) {
                exchangeControl(control);
            } else {
                release();
            }
        }

        constexpr RcuSharedPtr(rcu::detail::ControlBlock *control) : RcuSharedPtr() {
            acquire(control);
        }

    public:
        constexpr RcuSharedPtr()
            : mControl(nullptr)
        { }

        constexpr RcuSharedPtr(std::nullptr_t)
            : mControl(nullptr)
        { }

        constexpr ~RcuSharedPtr() {
            release();
        }

        constexpr RcuSharedPtr(RcuDomain *domain, T *ptr);

        constexpr RcuSharedPtr(const RcuSharedPtr& other) : RcuSharedPtr() {
            acquire(other.mControl);
        }

        constexpr RcuSharedPtr& operator=(const RcuSharedPtr& other) {
            acquire(other.mControl);
            return *this;
        }

        constexpr RcuSharedPtr(RcuSharedPtr&& other) : RcuSharedPtr() {
            acquire(other.mControl.exchange(nullptr));
        }

        constexpr RcuSharedPtr& operator=(RcuSharedPtr&& other) {
            acquire(other.mControl.exchange(nullptr));
            return *this;
        }

        RcuWeakPtr<T> weak();

        constexpr T *operator->() {
            rcu::detail::ControlBlock *cb = mControl.load();
            return static_cast<T*>(cb->value);
        }

        constexpr T *get() {
            rcu::detail::ControlBlock *cb = mControl.load();
            return static_cast<T*>(cb->value);
        }

        constexpr T& operator*(this auto&& self) {
            rcu::detail::ControlBlock *cb = self.mControl.load();
            return *static_cast<T*>(cb->value);
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
        std::atomic<rcu::detail::ControlBlock*> mControl;

        void exchangeControl(rcu::detail::ControlBlock *control) {
            if (rcu::detail::ControlBlock *cb = mControl.exchange(control)) {
                rcu::detail::RcuReleaseWeak(cb);
            }
        }

        void release() {
            exchangeControl(nullptr);
        }

        void acquire(rcu::detail::ControlBlock *control) {
            if (control != nullptr && rcu::detail::RcuAcquireWeak(control)) {
                exchangeControl(control);
            } else {
                release();
            }
        }

    public:
        constexpr RcuWeakPtr()
            : mControl(nullptr)
        { }

        constexpr RcuWeakPtr(std::nullptr_t)
            : mControl(nullptr)
        { }

        constexpr ~RcuWeakPtr() {
            release();
        }

        constexpr RcuWeakPtr(const RcuSharedPtr<T>& shared) : RcuWeakPtr() {
            acquire(shared.mControl);
        }

        constexpr RcuWeakPtr(const RcuWeakPtr& other) : RcuWeakPtr() {
            acquire(other.mControl);
        }

        constexpr RcuWeakPtr& operator=(const RcuWeakPtr& other) {
            acquire(other.mControl);

            return *this;
        }

        constexpr RcuWeakPtr(RcuWeakPtr&& other) : RcuWeakPtr() {
            acquire(other.mControl.exchange(nullptr));
        }

        constexpr RcuWeakPtr& operator=(RcuWeakPtr&& other) {
            acquire(other.mControl.exchange(nullptr));

            return *this;
        }

        RcuSharedPtr<T> lock() {
            return RcuSharedPtr<T>(mControl);
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
        : mControl(new (std::nothrow) rcu::detail::ControlBlock { { 1, 1 }, ptr, [](void *ptr) { delete static_cast<T*>(ptr); }, domain })
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
