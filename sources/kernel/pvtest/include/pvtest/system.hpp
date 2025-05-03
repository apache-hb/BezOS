#pragma once

#include <stddef.h>

namespace pv {
    void *SharedObjectMalloc(size_t size);
    void *SharedObjectAlignedAlloc(size_t align, size_t size);
    void SharedObjectFree(void *ptr);

    template<typename T>
    struct SharedAllocator {
        using value_type = T;

        SharedAllocator() = default;

        template<typename U>
        SharedAllocator(const SharedAllocator<U> &) { }

        T *allocate(size_t n) {
            return static_cast<T *>(SharedObjectAlignedAlloc(alignof(T), n * sizeof(T)));
        }

        void deallocate(T *ptr, size_t) noexcept {
            SharedObjectFree(ptr);
        }

        constexpr bool operator==(const SharedAllocator &) const noexcept {
            return true;
        }
    };
}

#define PVTEST_SHARED_OBJECT(name) \
    static void *operator new(size_t size) { return pv::SharedObjectAlignedAlloc(alignof(name), size); } \
    static void *operator new[](size_t size) { return pv::SharedObjectAlignedAlloc(alignof(name), size); } \
    static void operator delete(void *ptr) noexcept { pv::SharedObjectFree(ptr); } \
    static void operator delete[](void *ptr) noexcept { pv::SharedObjectFree(ptr); }
