#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>

namespace sm {
    namespace detail {
        struct ControlBlock {
            std::atomic<size_t> count;
            void *value;
        };

        struct BaseIntrusiveCount {
            std::atomic<uint32_t> mIntrusiveCount{1};
        };
    }

    template<typename T>
    class SharedPtr {
        template<typename U>
        friend class SharedPtr;

        detail::ControlBlock *mControl;

        // TODO: figure out which memory ordering is optimal
        // im too scared to use anything other than seq_cst

        void acquire() {
            if (mControl != nullptr) {
                mControl->count.fetch_add(1, std::memory_order_seq_cst);
            }
        }

        void release() {
            if (mControl != nullptr && mControl->count.fetch_sub(1, std::memory_order_seq_cst) == 1) {
                delete get();
                delete mControl;
            }
        }

    public:
        SharedPtr()
            : mControl(nullptr)
        { }

        SharedPtr(T *value) : SharedPtr() {
            if (value != nullptr) {
                mControl = new detail::ControlBlock { 1, value };
            }
        }

        template<std::derived_from<T> U>
        SharedPtr(U *value) : SharedPtr() {
            if (value != nullptr) {
                mControl = new detail::ControlBlock { 1, value };
            }
        }

        template<std::derived_from<T> U>
        SharedPtr(const SharedPtr<U>& other)
            : mControl(other.mControl)
        {
            acquire();
        }

        SharedPtr(const SharedPtr& other)
            : mControl(other.mControl)
        {
            acquire();
        }

        SharedPtr(SharedPtr&& other)
            : mControl(other.mControl)
        {
            other.mControl = nullptr;
        }

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

        bool operator==(const SharedPtr& other) const {
            if (mControl == other.mControl) {
                return true;
            }

            if (mControl != nullptr && other.mControl != nullptr) {
                return *(T*)mControl->value == *(T*)other.mControl->value;
            }

            return false;
        }

        bool operator!=(const SharedPtr& other) const {
            return !(*this == other);
        }
    };

    template<std::derived_from<detail::BaseIntrusiveCount> T>
    class SharedPtr<T> {
        T *mObject;

        void acquire();

        void release();

        SharedPtr(T *value)
            : mObject(value)
        {
            acquire();
        }

    public:
        SharedPtr()
            : mObject(nullptr)
        { }

        SharedPtr(const SharedPtr& other)
            : SharedPtr(other.mObject)
        { }

        SharedPtr(SharedPtr&& other)
            : mObject(other.mObject)
        {
            other.mObject = nullptr;
        }

        SharedPtr& operator=(const SharedPtr& other) {
            if (mObject != other.mObject) {
                release();
                mObject = other.mObject;
                acquire();
            }

            return *this;
        }

        SharedPtr& operator=(SharedPtr&& other) {
            if (mObject != other.mObject) {
                release();
                mObject = other.mObject;
                other.mObject = nullptr;
            }

            return *this;
        }

        ~SharedPtr() {
            release();
        }

        T *operator->() const {
            return get();
        }

        T *get() const {
            return mObject;
        }

        T& operator*() const {
            return *get();
        }

        bool operator==(const SharedPtr& other) const {
            if (mObject == other.mObject) {
                return true;
            }

            if (mObject != nullptr && other.mObject != nullptr) {
                return *mObject == *other.mObject;
            }

            return false;
        }

        bool operator!=(const SharedPtr& other) const {
            return !(*this == other);
        }
    };

    template<typename T>
    class IntrusiveCount : private detail::BaseIntrusiveCount {
        friend class SharedPtr<T>;

    protected:
        SharedPtr<T> share() {
            return SharedPtr<T>(static_cast<T*>(this));
        }
    };

    template<std::derived_from<detail::BaseIntrusiveCount> T, typename... Args>
    SharedPtr<T> makeShared(Args&&... args) {
        return SharedPtr<T>(new T(std::forward<Args>(args)...));
    }
}
