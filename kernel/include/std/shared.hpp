#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>

namespace sm {
    namespace detail {
        struct ControlBlock {
            std::atomic<size_t> count;
            void *value;
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
}
