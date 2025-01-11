#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"

namespace mem {
    class BumpAllocator final : public mem::IAllocator {
        using Super = mem::IAllocator;

        void *mFront;
        void *mBack;

        void *mCurrent;

    public:
        using Super::Super;

        constexpr BumpAllocator(void *front, void *back)
            : mFront(front)
            , mBack(back)
            , mCurrent(front)
        { }

        void *allocate(size_t size, size_t align) override {
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

        void deallocate(void *ptr, size_t size) override {
            if (ptr == mCurrent) {
                mCurrent = (void*)((size_t)mCurrent - size);
            }
        }

        void reset() {
            mCurrent = mFront;
        }

        const void *front() const { return mFront; }
        const void *back() const { return mBack; }
    };
}
