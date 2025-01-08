#pragma once

#include <stddef.h>

#include <utility>

namespace km {
    class Arena {
        void *mFront;
        void *mBack;

        void *mCurrent;

    public:
        constexpr Arena(void *front, void *back)
            : mFront(front)
            , mBack(back)
            , mCurrent(front)
        { }

        void *allocate(size_t size, size_t align = alignof(size_t)) {
            size_t offset = (size_t)mCurrent % align;
            if (offset != 0) {
                mCurrent = (void*)((size_t)mCurrent + align - offset);
            }

            void *ptr = mCurrent;
            mCurrent = (void*)((size_t)mCurrent + size);

            if (mCurrent > mBack) {
                return nullptr;
            }

            return ptr;
        }

        void reset() {
            mCurrent = mFront;
        }

        template<typename T>
        T *allocate(auto&&... args) {
            if (void *ptr = allocate(sizeof(T), alignof(T))) {
                return new (ptr) T(std::forward<decltype(args)>(args)...);
            }

            return nullptr;
        }

        template<typename T>
        void deallocate(T *ptr) {
            ptr->~T();
        }
    };
}
