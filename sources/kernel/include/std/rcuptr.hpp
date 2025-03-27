#pragma once

#include "std/rcu.hpp"

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
            JointCount count;
            void *value;
            void(*deleter)(void*);
            RcuDomain *domain;
        };

        template<typename T>
        ControlBlock *NewControlBlock(RcuDomain *domain, T *value) {
            return new (std::nothrow) ControlBlock {
                .count = { 1, 1 },
                .value = value,
                .deleter = [](void *ptr) {
                    ControlBlock *cb = (ControlBlock*)ptr;
                    delete static_cast<T*>(cb->value);
                },
                .domain = domain
            };
        }

        void RcuReleaseStrong(ControlBlock *ptr);
        bool RcuAcqiureStrong(ControlBlock *control);
        void RcuReleaseWeak(ControlBlock *cb);
        bool RcuAcquireWeak(ControlBlock *control);

        struct AdoptControl {};
        struct AcquireControl {};
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

        template<typename O>
        friend class RcuSharedPtr;

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

        constexpr RcuSharedPtr(rcu::detail::ControlBlock *control, rcu::detail::AcquireControl) : RcuSharedPtr() {
            acquire(control);
        }

        constexpr RcuSharedPtr(rcu::detail::ControlBlock *control, rcu::detail::AdoptControl) : RcuSharedPtr() {
            exchangeControl(control);
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

        /// @brief Construct a shared pointer from a domain and a pointer.
        ///
        /// @note Check for validity with @a isValid() after construction to ensure
        ///       allocation of control structures succeeded.
        ///
        /// @pre @p ptr must not be associated with any other shared pointers.
        ///
        /// @param domain The domain that reclaims the pointer.
        /// @param ptr The pointer to manage.
        constexpr RcuSharedPtr(RcuDomain *domain [[gnu::nonnull]], T *ptr);

        /// @brief Copy construct a shared pointer.
        ///
        /// @param other The shared pointer to copy.
        constexpr RcuSharedPtr(const RcuSharedPtr& other) : RcuSharedPtr() {
            acquire(other.mControl);
        }

        /// @brief Copy a shared pointer into this shared pointer.
        ///
        /// @note Does not require external synchronization
        ///
        /// @param other The shared pointer to copy.
        /// @return A reference to this shared pointer.
        constexpr RcuSharedPtr& operator=(const RcuSharedPtr& other) {
            acquire(other.mControl);
            return *this;
        }

        /// @brief Move construct a shared pointer.
        ///
        /// @post @p other is empty.
        ///
        /// @param other The shared pointer to move.
        constexpr RcuSharedPtr(RcuSharedPtr&& other) : RcuSharedPtr() {
            exchangeControl(other.mControl.exchange(nullptr));
        }

        /// @brief Move a shared pointer into this shared pointer.
        ///
        /// @note Does not require external synchronization
        /// @post @p other is empty.
        ///
        /// @param other The shared pointer to move.
        /// @return A reference to this shared pointer.
        constexpr RcuSharedPtr& operator=(RcuSharedPtr&& other) {
            exchangeControl(other.mControl.exchange(nullptr));
            return *this;
        }

        /// @brief Take a weak reference to this shared pointer.
        /// @note Does not require external synchronization
        /// @return A weak pointer to this shared pointer.
        RcuWeakPtr<T> weak();

        /// @brief Compare and exchange the shared pointer.
        ///
        /// @note Does not require external synchronization
        ///
        /// @param expected The expected shared pointer.
        /// @param desired The desired shared pointer.
        /// @return True if the shared pointer was exchanged, false otherwise.
        constexpr bool compare_exchange_strong(RcuSharedPtr& expected, RcuSharedPtr desired) {
            rcu::detail::ControlBlock *oldControl = mControl.load();
            rcu::detail::ControlBlock *expectedControl = expected.mControl.load();
            rcu::detail::ControlBlock *desiredControl = desired.mControl.load();

            if (!mControl.compare_exchange_strong(expectedControl, desiredControl)) {
                expected = RcuSharedPtr(expectedControl, rcu::detail::AcquireControl{});
                return false;
            }

            if (oldControl != nullptr) {
                rcu::detail::RcuReleaseStrong(oldControl);
            }

            return true;
        }

        /// @brief Compare and exchange the shared pointer.
        ///
        /// @note Does not require external synchronization
        ///
        /// @param expected The expected shared pointer.
        /// @param desired The desired shared pointer.
        /// @return True if the shared pointer was exchanged, false otherwise.
        constexpr bool compare_exchange_weak(RcuSharedPtr& expected, RcuSharedPtr desired) {
            rcu::detail::ControlBlock *oldControl = mControl.load();
            rcu::detail::ControlBlock *expectedControl = expected.mControl.load();
            rcu::detail::ControlBlock *desiredControl = desired.mControl.load();

            if (!mControl.compare_exchange_weak(expectedControl, desiredControl)) {
                expected = RcuSharedPtr(expectedControl, rcu::detail::AcquireControl{});
                return false;
            }

            if (oldControl != nullptr) {
                rcu::detail::RcuReleaseStrong(oldControl);
            }

            return true;
        }

        constexpr bool cmpxchg(RcuSharedPtr& expected, RcuSharedPtr desired) {
            return compare_exchange_strong(expected, desired);
        }

        constexpr bool cmpxchgStrong(RcuSharedPtr& expected, RcuSharedPtr desired) {
            return compare_exchange_strong(expected, desired);
        }

        constexpr bool cmpxchgWeak(RcuSharedPtr& expected, RcuSharedPtr desired) {
            return compare_exchange_weak(expected, desired);
        }

        constexpr RcuSharedPtr exchange(RcuSharedPtr desired) {
            return RcuSharedPtr(mControl.exchange(desired.mControl), rcu::detail::AdoptControl{});
        }

        constexpr RcuSharedPtr load() {
            return *this;
        }

        constexpr void store(RcuSharedPtr desired) {
            acquire(desired.mControl);
        }

        /// @brief Access the shared pointers value
        /// @note Requires external synchronization
        /// @pre @a isValid()
        constexpr T *operator->() {
            rcu::detail::ControlBlock *cb = mControl.load();
            return static_cast<T*>(cb->value);
        }

        /// @brief Access the shared pointers value
        /// @note Requires external synchronization
        constexpr T *get() {
            if (rcu::detail::ControlBlock *cb = mControl.load()) {
                return static_cast<T*>(cb->value);
            }

            return nullptr;
        }

        /// @brief Access the shared pointers value
        /// @note Requires external synchronization
        /// @pre @a isValid()
        constexpr T& operator*(this auto&& self) {
            rcu::detail::ControlBlock *cb = self.mControl.load();
            return *static_cast<T*>(cb->value);
        }

        /// @brief Erase the shared pointers value
        /// @note Does not require external synchronization
        void reset() {
            release();
        }

        /// @brief Replace the pointer with a new value
        /// @note Does not require external synchronization
        [[nodiscard("Verify allocation succeeded")]]
        bool reset(RcuDomain *domain [[gnu::nonnull]], T *ptr) {
            if (rcu::detail::ControlBlock *cb = rcu::detail::NewControlBlock(domain, ptr)) {
                exchangeControl(cb);
                return true;
            }

            return false;
        }

        /// @brief Check if the shared pointer contains a value.
        /// @note Does not require external synchronization
        /// @return True if the shared pointer contains a value, false otherwise.
        constexpr operator bool() const {
            return mControl != nullptr;
        }

        /// @brief Check if the shared pointer contains a value.
        /// @note Does not require external synchronization
        /// @return True if the shared pointer contains a value, false otherwise.
        constexpr bool isValid() const {
            return mControl != nullptr;
        }

        /// @brief Check if two shared pointers point to the same object.
        /// @note Does not require external synchronization
        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuSharedPtr<O>& other) const {
            return mControl == other.mControl;
        }

        constexpr bool operator==(std::nullptr_t) const {
            return mControl == nullptr;
        }

        constexpr size_t hash() const {
            return std::hash<rcu::detail::ControlBlock*>{}(mControl);
        }
    };

    template<typename T>
    class RcuWeakPtr {
        template<typename O>
        friend class RcuWeakPtr;

        template<typename O>
        friend class RcuSharedPtr;

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

        constexpr RcuWeakPtr(const RcuWeakPtr& other) : RcuWeakPtr() {
            acquire(other.mControl);
        }

        constexpr RcuWeakPtr(RcuWeakPtr&& other) : RcuWeakPtr() {
            exchangeControl(other.mControl.exchange(nullptr));
        }

        template<std::derived_from<T> O>
        constexpr RcuWeakPtr(const RcuWeakPtr<O>& other) : RcuWeakPtr() {
            acquire(other.mControl);
        }

        constexpr RcuWeakPtr& operator=(const RcuWeakPtr& other) {
            acquire(other.mControl);
            return *this;
        }

        constexpr RcuWeakPtr& operator=(RcuWeakPtr&& other) {
            exchangeControl(other.mControl.exchange(nullptr));
            return *this;
        }

        constexpr RcuWeakPtr(const RcuSharedPtr<T>& shared) : RcuWeakPtr() {
            acquire(shared.mControl);
        }

        template<std::derived_from<T> O>
        constexpr RcuWeakPtr(const RcuSharedPtr<O>& shared) : RcuWeakPtr() {
            acquire(shared.mControl);
        }

        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuWeakPtr<O>& other) const noexcept {
            return mControl == other.mControl;
        }

        RcuSharedPtr<T> lock() {
            return RcuSharedPtr<T>(mControl, rcu::detail::AcquireControl{});
        }

        constexpr size_t hash() const {
            return std::hash<rcu::detail::ControlBlock*>{}(mControl);
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
    RcuSharedPtr<T> rcuMakeShared(RcuDomain *domain [[gnu::nonnull]], Args&&... args) {
        if (T *ptr = new (std::nothrow) T(std::forward<Args>(args)...)) {
            if (RcuSharedPtr rcu = RcuSharedPtr<T>(domain, ptr)) {
                return rcu;
            }

            delete ptr;
        }

        return nullptr;
    }

    template<typename T>
    constexpr RcuSharedPtr<T>::RcuSharedPtr(RcuDomain *domain [[gnu::nonnull]], T *ptr) {
        //
        // If there is no pointer then dont bother creating a control block.
        //
        if (ptr == nullptr) return;

        if (rcu::detail::ControlBlock *control = rcu::detail::NewControlBlock(domain, ptr)) {
            mControl.store(control);

            //
            // If the control block was allocated and the pointed type has enabled
            // shared from this then setup its weak reference to itself.
            //
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
        return ptr.hash();
    }
};

template<typename T>
struct std::hash<sm::RcuWeakPtr<T>> {
    constexpr size_t operator()(const sm::RcuWeakPtr<T>& ptr) const noexcept {
        return ptr.hash();
    }
};
