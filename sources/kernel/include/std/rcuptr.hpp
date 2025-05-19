#pragma once

#include "std/detail/sticky_counter.hpp"

#include "std/rcu.hpp"

namespace sm {
    namespace rcu::detail {
        class JointCount {
            sm::detail::WaitFreeCounter<uint32_t> mWeakCount;
            sm::detail::WaitFreeCounter<uint32_t> mStrongCount;

        public:
            enum Release { eNone = 0, eStrong = (1 << 0), eWeak = (1 << 1) };

            constexpr JointCount() noexcept = default;
            constexpr JointCount(uint32_t strong, uint32_t weak) noexcept
                : mWeakCount(weak)
                , mStrongCount(strong)
            { }

            /// @brief Increment the weak reference count.
            /// @return False if the strong reference count is zero, true otherwise.
            bool weakRetain() noexcept [[clang::reentrant, clang::nonblocking]] {
                return mWeakCount.increment(1);
            }

            /// @brief Decrement the weak reference count.
            /// @return True if the weak count was brought to zero by this operation, false otherwise.
            bool weakRelease() noexcept [[clang::reentrant, clang::nonblocking]] {
                return mWeakCount.decrement(1);
            }

            /// @brief Increment the weak and strong reference count.
            /// @return False if the strong reference count is zero, true otherwise.
            bool strongRetain() noexcept [[clang::reentrant, clang::nonblocking]] {
                if (mStrongCount.increment(1)) {
                    weakRetain();
                    return true;
                }

                return false;
            }

            /// @brief Decrement the weak and strong reference count.
            /// @return A mask of which reference counts were brought to zero by this operation.
            Release strongRelease() noexcept [[clang::reentrant, clang::nonblocking]] {
                bool strongCleared = mStrongCount.decrement(1);
                bool weakCleared = mWeakCount.decrement(1);

                int result = eNone;
                if (strongCleared)
                    result |= eStrong;
                if (weakCleared)
                    result |= eWeak;

                return Release(result);
            }

            uint32_t strongCount(std::memory_order order = std::memory_order_seq_cst) const noexcept [[clang::reentrant, clang::nonblocking]] {
                return mStrongCount.load(order);
            }

            uint32_t weakCount(std::memory_order order = std::memory_order_seq_cst) const noexcept [[clang::reentrant, clang::nonblocking]] {
                return mWeakCount.load(order);
            }
        };

        UTIL_BITFLAGS(JointCount::Release);

        /// @brief Token for deferred reclamation.
        /// @details Object reclamation can be seperate from control block reclamation,
        ///          to ensure we never allocate new RcuObjects in the destructor of
        ///          a RcuSharedPtr we need to allocate this token ahead of time.
        struct RcuToken : public RcuObject {
            void *value{nullptr};

            RcuToken(void *v) noexcept
                : RcuObject()
                , value(v)
            { }
        };

        struct ControlBlock : public RcuObject {
            JointCount count;
            void *value;
            void(*deleter)(void*);
            RcuDomain *domain;
            RcuToken *token;

            ControlBlock(void *v, void (*d)(void*), RcuDomain *r, RcuToken *t)
                : RcuObject()
                , count(1, 1)
                , value(v)
                , deleter(d)
                , domain(r)
                , token(t)
            { }
        };

        template<typename T>
        ControlBlock *NewControlBlock(RcuDomain *domain, T *value) {
            void (*finalize)(void*) = [](void *ptr) {
                RcuToken *token = static_cast<RcuToken*>(ptr);
                delete static_cast<T*>(token->value);
                delete token;
            };

            if (RcuToken *token = new (std::nothrow) RcuToken(value)) {
                if (ControlBlock *control = new (std::nothrow) ControlBlock(value, finalize, domain, token)) {
                    return control;
                }
                delete token;
            }

            return nullptr;
        }

        void RcuReleaseStrong(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]];
        bool RcuAcqiureStrong(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]];
        void RcuReleaseWeak(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]];
        bool RcuAcquireWeak(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]];

        class RcuIntrusivePtrTag {};

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
    concept IsIntrusivePtr = std::derived_from<T, rcu::detail::RcuIntrusivePtrTag>;

    template<typename O, typename Self>
    constexpr RcuSharedPtr<O> rcuSharedPtrCast(const RcuSharedPtr<Self>& weak) {
        if (weak.mControl != nullptr) {
            return RcuSharedPtr<O>(weak.mControl, rcu::detail::AcquireControl{});
        }

        return nullptr;
    }

    template<typename O, typename Self>
    constexpr RcuWeakPtr<O> rcuWeakPtrCast(const RcuWeakPtr<Self>& weak) {
        if (weak.mControl != nullptr) {
            return RcuWeakPtr<O>(weak.mControl, rcu::detail::AcquireControl{});
        }

        return nullptr;
    }

    /// @brief A shared pointer using RCU for deferred reclamation.
    ///
    /// Class invariants:
    /// * if mControl->strongCount() > 0 the object is accessible
    /// * if mControl->strongCount() == 0 the object is inaccessible
    /// * if mControl->weakCount() > 0 the control block is accessible
    /// * mControl->weakCount() must only be 0 once, the holder that decrements to 0 must release the object *and* must be the last holder.
    template<typename T>
    class RcuSharedPtr {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>>, "Cannot use RcuSharedPtr with reference types");
        static_assert(!std::is_same_v<T, rcu::detail::ControlBlock>, "Cannot use RcuSharedPtr with ControlBlock");

        template<typename O>
        friend class RcuWeakPtr;

        template<typename O>
        friend class RcuSharedPtr;

        std::atomic<rcu::detail::ControlBlock*> mControl;

        void replaceControl(rcu::detail::ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (rcu::detail::ControlBlock *old = mControl.exchange(control)) {
                rcu::detail::RcuReleaseStrong(old);
            }
        }

        void release() noexcept [[clang::reentrant, clang::nonblocking]] {
            replaceControl(nullptr);
        }

        void acquire(rcu::detail::ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (control != nullptr && rcu::detail::RcuAcqiureStrong(control)) {
                replaceControl(control);
            } else {
                release();
            }
        }

        constexpr RcuSharedPtr(rcu::detail::ControlBlock *control, rcu::detail::AcquireControl) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            acquire(control);
        }

        constexpr RcuSharedPtr(rcu::detail::ControlBlock *control, rcu::detail::AdoptControl) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            replaceControl(control);
        }

    public:
        constexpr RcuSharedPtr() noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        constexpr RcuSharedPtr(std::nullptr_t) noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        constexpr ~RcuSharedPtr() noexcept [[clang::reentrant, clang::nonblocking]] {
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
        constexpr RcuSharedPtr(RcuDomain *domain [[gnu::nonnull]], T *ptr) noexcept [[clang::nonreentrant, clang::allocating]];

        /// @brief Copy construct a shared pointer.
        ///
        /// @param other The shared pointer to copy.
        [[nodiscard]]
        constexpr RcuSharedPtr(const RcuSharedPtr& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            acquire(other.mControl);
        }

        /// @brief Copy a shared pointer into this shared pointer.
        ///
        /// @note Does not require external synchronization
        ///
        /// @param other The shared pointer to copy.
        /// @return A reference to this shared pointer.
        constexpr RcuSharedPtr& operator=(const RcuSharedPtr& other) noexcept [[clang::reentrant, clang::nonblocking]] {
            acquire(other.mControl);
            return *this;
        }

        /// @brief Move construct a shared pointer.
        ///
        /// @post @p other is empty.
        ///
        /// @param other The shared pointer to move.
        [[nodiscard]]
        constexpr RcuSharedPtr(RcuSharedPtr&& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            replaceControl(other.mControl.exchange(nullptr));
        }

        /// @brief Move a shared pointer into this shared pointer.
        ///
        /// @note Does not require external synchronization
        /// @post @p other is empty.
        ///
        /// @param other The shared pointer to move.
        /// @return A reference to this shared pointer.
        constexpr RcuSharedPtr& operator=(RcuSharedPtr&& other) noexcept [[clang::reentrant, clang::nonblocking]] {
            replaceControl(other.mControl.exchange(nullptr));
            return *this;
        }

        template<std::derived_from<T> O>
        [[nodiscard]]
        constexpr RcuSharedPtr(const RcuSharedPtr<O>& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            acquire(other.mControl);
        }

        template<std::derived_from<T> O>
        [[nodiscard]]
        constexpr RcuSharedPtr(RcuSharedPtr<O>&& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuSharedPtr()
        {
            replaceControl(other.mControl.exchange(nullptr));
        }

        /// @brief Take a weak reference to this shared pointer.
        /// @note Does not require external synchronization
        /// @return A weak pointer to this shared pointer.
        RcuWeakPtr<T> weak() noexcept [[clang::reentrant, clang::nonblocking]];

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
        constexpr bool compare_exchange_weak(RcuSharedPtr& expected, const RcuSharedPtr& desired) {
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

        constexpr bool cmpxchg(RcuSharedPtr& expected, const RcuSharedPtr& desired) {
            return compare_exchange_strong(expected, desired);
        }

        constexpr bool cmpxchgStrong(RcuSharedPtr& expected, const RcuSharedPtr& desired) {
            return compare_exchange_strong(expected, desired);
        }

        constexpr bool cmpxchgWeak(RcuSharedPtr& expected, const RcuSharedPtr& desired) {
            return compare_exchange_weak(expected, desired);
        }

        constexpr RcuSharedPtr exchange(const RcuSharedPtr& desired) {
            return RcuSharedPtr(mControl.exchange(desired.mControl), rcu::detail::AdoptControl{});
        }

        constexpr RcuSharedPtr load() {
            return *this;
        }

        constexpr void store(RcuDomain *domain, const RcuSharedPtr& desired) {
            sm::RcuGuard guard(*domain);
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
        void reset() noexcept [[clang::reentrant, clang::nonblocking]] {
            release();
        }

        /// @brief Replace the pointer with a new value
        /// @note Does not require external synchronization
        [[nodiscard("Verify allocation succeeded")]]
        bool reset(RcuDomain *domain [[gnu::nonnull]], T *ptr) noexcept [[clang::nonreentrant, clang::allocating]] {
            if (rcu::detail::ControlBlock *cb = rcu::detail::NewControlBlock(domain, ptr)) {
                replaceControl(cb);
                return true;
            }

            return false;
        }

        /// @brief Check if the shared pointer contains a value.
        /// @note Does not require external synchronization
        /// @return True if the shared pointer contains a value, false otherwise.
        constexpr operator bool() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mControl != nullptr;
        }

        /// @brief Check if the shared pointer contains a value.
        /// @note Does not require external synchronization
        /// @return True if the shared pointer contains a value, false otherwise.
        constexpr bool isValid() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mControl != nullptr;
        }

        /// @brief Check if two shared pointers point to the same object.
        /// @note Does not require external synchronization
        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuSharedPtr<O>& other) const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mControl == other.mControl;
        }

        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuWeakPtr<O>& other) const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mControl == other.mControl;
        }

        constexpr bool operator==(std::nullptr_t) const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mControl == nullptr;
        }

        constexpr size_t hash() const {
            return std::hash<rcu::detail::ControlBlock*>{}(mControl);
        }

        constexpr size_t strongCount() const noexcept [[clang::reentrant, clang::nonblocking]] {
            if (rcu::detail::ControlBlock *cb = mControl.load()) {
                return cb->count.strongCount();
            }

            return 0;
        }

        constexpr size_t weakCount() const noexcept [[clang::reentrant, clang::nonblocking]] {
            if (rcu::detail::ControlBlock *cb = mControl.load()) {
                return cb->count.weakCount();
            }

            return 0;
        }

        template<typename O, typename Self>
        friend constexpr RcuSharedPtr<O> sm::rcuSharedPtrCast(const RcuSharedPtr<Self>& weak);
    };

    template<typename T>
    class RcuWeakPtr {
        template<typename O>
        friend class RcuWeakPtr;

        template<typename O>
        friend class RcuSharedPtr;

        std::atomic<rcu::detail::ControlBlock*> mControl;

        void exchangeControl(rcu::detail::ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (rcu::detail::ControlBlock *cb = mControl.exchange(control)) {
                rcu::detail::RcuReleaseWeak(cb);
            }
        }

        void release() noexcept [[clang::reentrant, clang::nonblocking]] {
            exchangeControl(nullptr);
        }

        void acquire(rcu::detail::ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
            if (control != nullptr && rcu::detail::RcuAcquireWeak(control)) {
                exchangeControl(control);
            } else {
                release();
            }
        }

        constexpr RcuWeakPtr(rcu::detail::ControlBlock *control, rcu::detail::AcquireControl) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuWeakPtr()
        {
            acquire(control);
        }

    public:
        constexpr RcuWeakPtr() noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        constexpr RcuWeakPtr(std::nullptr_t) noexcept [[clang::reentrant, clang::nonblocking]]
            : mControl(nullptr)
        { }

        constexpr ~RcuWeakPtr() noexcept [[clang::reentrant, clang::nonblocking]] {
            release();
        }

        constexpr RcuWeakPtr(const RcuWeakPtr& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuWeakPtr()
        {
            acquire(other.mControl);
        }

        constexpr RcuWeakPtr(RcuWeakPtr&& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuWeakPtr()
        {
            exchangeControl(other.mControl.exchange(nullptr));
        }

        template<std::derived_from<T> O>
        constexpr RcuWeakPtr(const RcuWeakPtr<O>& other) noexcept [[clang::reentrant, clang::nonblocking]]
            : RcuWeakPtr()
        {
            acquire(other.mControl);
        }

        template<std::derived_from<T> O>
        constexpr RcuWeakPtr(RcuWeakPtr<O>&& other) : RcuWeakPtr() {
            exchangeControl(other.mControl.exchange(nullptr));
        }

        constexpr RcuWeakPtr& operator=(const RcuWeakPtr& other) noexcept [[clang::reentrant, clang::nonblocking]] {
            acquire(other.mControl);
            return *this;
        }

        constexpr RcuWeakPtr& operator=(RcuWeakPtr&& other) noexcept [[clang::reentrant, clang::nonblocking]] {
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
        constexpr RcuWeakPtr(RcuSharedPtr<O>&& shared) : RcuWeakPtr() {
            exchangeControl(shared.mControl.exchange(nullptr));
        }

        RcuSharedPtr<T> lock() {
            return RcuSharedPtr<T>(mControl, rcu::detail::AcquireControl{});
        }

        constexpr size_t hash() const {
            return std::hash<rcu::detail::ControlBlock*>{}(mControl);
        }

        size_t strongCount() const {
            if (rcu::detail::ControlBlock *cb = mControl.load()) {
                return cb->count.strongCount();
            }

            return 0;
        }

        size_t weakCount() const {
            if (rcu::detail::ControlBlock *cb = mControl.load()) {
                return cb->count.weakCount();
            }

            return 0;
        }

        void reset() {
            release();
        }

        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuSharedPtr<O>& other) const {
            return mControl == other.mControl;
        }

        template<std::derived_from<T> O>
        constexpr bool operator==(const RcuWeakPtr<O>& other) const {
            return mControl == other.mControl;
        }

        constexpr bool operator==(std::nullptr_t) const {
            return mControl == nullptr;
        }

        template<typename O, typename Self>
        friend constexpr RcuWeakPtr<O> sm::rcuWeakPtrCast(const RcuWeakPtr<Self>& weak);
    };

    template<typename T>
    struct RcuHash {
        using is_transparent = void;

        constexpr size_t operator()(const RcuSharedPtr<T>& ptr) const noexcept {
            return ptr.hash();
        }
        constexpr size_t operator()(const RcuWeakPtr<T>& ptr) const noexcept {
            return ptr.hash();
        }
    };

    template<typename T>
    class RcuIntrusivePtr : public rcu::detail::RcuIntrusivePtrTag {
        template<typename O>
        friend class RcuSharedPtr;

        template<typename O>
        friend class RcuWeakPtr;

        RcuWeakPtr<T> mWeakThis;

        void initWeak(RcuWeakPtr<T> weak) {
            mWeakThis = weak;
        }

    public:
        template<typename Self>
        auto loanWeak(this Self&& self) -> RcuWeakPtr<std::remove_cvref_t<Self>> {
            return rcuWeakPtrCast<std::remove_cvref_t<Self>>(self.mWeakThis);
        }

        template<typename Self>
        auto loanShared(this Self&& self) -> RcuSharedPtr<std::remove_cvref_t<Self>> {
            return rcuSharedPtrCast<std::remove_cvref_t<Self>>(self.mWeakThis.lock());
        }
    };

    template<typename T, typename... Args>
    RcuSharedPtr<T> rcuMakeShared(RcuDomain *domain [[gnu::nonnull]], Args&&... args) noexcept [[clang::nonreentrant, clang::allocating]] {
        if (T *ptr = new (std::nothrow) T(std::forward<Args>(args)...)) {
            if (RcuSharedPtr rcu = RcuSharedPtr<T>(domain, ptr)) {
                return rcu;
            }

            delete ptr;
        }

        return nullptr;
    }

    template<typename T>
    constexpr RcuSharedPtr<T>::RcuSharedPtr(RcuDomain *domain [[gnu::nonnull]], T *ptr) noexcept [[clang::nonreentrant, clang::allocating]] {
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
    RcuWeakPtr<T> RcuSharedPtr<T>::weak() noexcept [[clang::reentrant, clang::nonblocking]] {
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
