#pragma once

#include <atomic>
#include <climits>
#include <concepts>
#include <cstddef>
#include <utility>

// @todo: this is all very wrong and not correct

namespace sm {
    namespace detail {
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
            AtomicStickyZero<size_t> strong;
            AtomicStickyZero<size_t> weak;

            void *value;
            void (*deleter)(void*);
        };

        struct BaseIntrusiveCount {

        };

        inline bool AcquireWeak(ControlBlock& control) {
            return control.weak.increment();
        }

        inline bool ReleaseWeak(ControlBlock& control) {
            return control.weak.decrement();
        }

        inline bool AcquireStrong(ControlBlock& control) {
            //
            // Acquire the implicit weak reference for this strong reference.
            //
            AcquireWeak(control);

            //
            // If the strong ref counter has reached zero
            // then this control blocks pointed object is released.
            //
            if (!control.strong.increment()) {
                ReleaseWeak(control);
                return false;
            }

            return true;
        }

        inline bool ReleaseStrong(ControlBlock& control) {
            //
            // If we release the last strong counter then its our
            // responsibility to delete the managed data.
            //
            if (control.strong.decrement()) {
                control.deleter(control.value);
            }

            //
            // Also decrement the weak counter as each strong reference
            // has an implicit weak reference.
            //
            return ReleaseWeak(control);
        }
    }

    template<typename T>
    class WeakPtr;

    template<typename T>
    class SharedPtr;

    template<typename T>
    class WeakPtr {
        friend class SharedPtr<T>;

        detail::ControlBlock *mControl;

        void release() {
            if (mControl != nullptr) {
                //
                // If this was the last weak reference then it's our job
                // to cleanup the control block.
                //
                if (detail::ReleaseWeak(*mControl)) {
                    delete mControl;
                }
            }

            mControl = nullptr;
        }

        void acquire(detail::ControlBlock *control) {
            mControl = control;

            if (mControl) detail::AcquireWeak(*mControl);
        }

        WeakPtr(detail::ControlBlock *control) {
            if (control != nullptr) {
                if (!detail::AcquireWeak(*control)) {
                    control = nullptr;
                }
            }

            mControl = control;
        }

    public:
        WeakPtr() : mControl(nullptr) { }

        WeakPtr(std::nullptr_t)
            : mControl(nullptr)
        { }

        WeakPtr(const WeakPtr& other)
            : WeakPtr(other.mControl)
        { }

        WeakPtr(WeakPtr&& other)
            : mControl(std::exchange(other.mControl, nullptr))
        { }

        WeakPtr& operator=(const WeakPtr& other) {
            if (mControl != other.mControl) {
                release();
                acquire(other.mControl);
            }

            return *this;
        }

        WeakPtr& operator=(WeakPtr&& other) {
            release();
            mControl = std::exchange(other.mControl, nullptr);

            return *this;
        }

        ~WeakPtr() {
            release();
        }

        SharedPtr<T> lock() {
            return SharedPtr<T>(mControl);
        }
    };

    template<typename T>
    class SharedPtr {
        template<typename U>
        friend class SharedPtr;

        friend class WeakPtr<T>;

        detail::ControlBlock *mControl;

        void acquire() {
            if (mControl != nullptr) {
                if (!detail::AcquireStrong(*mControl)) {
                    mControl = nullptr;
                }
            }
        }

        void release() {
            if (mControl != nullptr) {
                if (detail::ReleaseStrong(*mControl)) {
                    delete mControl;
                }
            }

            mControl = nullptr;
        }

        SharedPtr(detail::ControlBlock *control) {
            if (control != nullptr) {
                if (!detail::AcquireStrong(*control)) {
                    control = nullptr;
                }
            }

            mControl = control;
        }

    public:
        SharedPtr()
            : mControl(nullptr)
        { }

        SharedPtr(std::nullptr_t)
            : mControl(nullptr)
        { }

        SharedPtr(T *value) : SharedPtr() {
            if (value != nullptr) {
                mControl = new detail::ControlBlock { 1, 1, value, +[](void *v) { delete static_cast<T*>(v); } };
            }
        }

        template<std::derived_from<T> U>
        SharedPtr(U *value) : SharedPtr() {
            if (value != nullptr) {
                mControl = new detail::ControlBlock { 1, 1, value, +[](void *v) { delete static_cast<T*>(v); } };
            }
        }

        template<std::derived_from<T> U>
        SharedPtr(const SharedPtr<U>& other)
            : SharedPtr(other.mControl)
        { }

        SharedPtr(const SharedPtr& other)
            : SharedPtr(other.mControl)
        { }

        SharedPtr(SharedPtr&& other)
            : mControl(std::exchange(other.mControl, nullptr))
        { }

        SharedPtr& operator=(const SharedPtr& other) {
            if (mControl != other.mControl) {
                release();
                mControl = other.mControl;
                acquire();
            }

            return *this;
        }

        SharedPtr& operator=(SharedPtr&& other) {
            if (mControl != other.mControl) {
                release();
                mControl = other.mControl;
                other.mControl = nullptr;
            }

            return *this;
        }

        ~SharedPtr() {
            release();
        }

        WeakPtr<T> weak() {
            return WeakPtr<T>(mControl);
        }

        T *operator->() const {
            return get();
        }

        T *get() const {
            if (mControl != nullptr) {
                return (T*)mControl->value;
            }

            return nullptr;
        }

        T& operator*() const {
            return *get();
        }

        constexpr friend bool operator==(const SharedPtr& lhs, const SharedPtr& rhs) {
            return lhs.mControl == rhs.mControl;
        }
    };

    template<typename T>
    class IntrusiveCount : private detail::BaseIntrusiveCount {
        friend class SharedPtr<T>;

        WeakPtr<T> mWeakThis;

    protected:
        SharedPtr<T> share() {
            return SharedPtr<T>(static_cast<T*>(this));
        }

        WeakPtr<T> weakShare() {
            return mWeakThis;
        }
    };

    template<std::derived_from<detail::BaseIntrusiveCount> T, typename... Args>
    SharedPtr<T> makeShared(Args&&... args) {
        return SharedPtr<T>(new T(std::forward<Args>(args)...));
    }
}
