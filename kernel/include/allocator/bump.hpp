#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"

namespace mem {
    class BumpAllocator : public mem::IAllocator {
        using Super = mem::IAllocator;

        void *mFront;
        void *mBack;

        void *mCurrent;

        bool wasLastAllocation(void *ptr, size_t size) const {
            return ((char*)ptr + size) == mCurrent;
        }

    public:
        using Super::Super;

        constexpr BumpAllocator(void *front, void *back)
            : mFront(front)
            , mBack(back)
            , mCurrent(front)
        { }

        void *allocate(size_t size) override {
            return allocateAligned(size, sizeof(std::max_align_t));
        }

        void *reallocate(void *old, size_t oldSize, size_t newSize) override {
            if (wasLastAllocation(old, oldSize)) {
                mCurrent = (char*)old + newSize;
                return old;
            }

            void *data = allocate(newSize);
            if (data != nullptr) {
                memcpy(data, old, oldSize);
            }

            return data;
        }

        void *allocateAligned(size_t size, size_t align) override {
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
            if (wasLastAllocation(ptr, size)) {
                mCurrent = ptr;
            }
        }

        void reset() {
            mCurrent = mFront;
        }

        const void *front() const { return mFront; }
        const void *back() const { return mBack; }
    };
}
