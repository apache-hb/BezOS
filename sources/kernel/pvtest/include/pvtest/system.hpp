#pragma once

#include <stddef.h>

namespace pv {
    void *sharedObjectMalloc(size_t size);
    void *sharedObjectAlignedAlloc(size_t align, size_t size);
    void sharedObjectFree(void *ptr);

    template<typename T>
    struct SharedAllocator {
        using value_type = T;

        SharedAllocator() = default;

        template<typename U>
        SharedAllocator(const SharedAllocator<U> &) { }

        T *allocate(size_t n) {
            return static_cast<T *>(sharedObjectAlignedAlloc(alignof(T), n * sizeof(T)));
        }

        void deallocate(T *ptr, size_t) noexcept {
            sharedObjectFree(ptr);
        }

        constexpr bool operator==(const SharedAllocator &) const noexcept {
            return true;
        }
    };
}

#define PVTEST_SHARED_OBJECT(name) \
    static void *operator new(size_t size) { return pv::sharedObjectAlignedAlloc(alignof(name), size); } \
    static void *operator new[](size_t size) { return pv::sharedObjectAlignedAlloc(alignof(name), size); } \
    static void operator delete(void *ptr) noexcept { pv::sharedObjectFree(ptr); } \
    static void operator delete[](void *ptr) noexcept { pv::sharedObjectFree(ptr); }
