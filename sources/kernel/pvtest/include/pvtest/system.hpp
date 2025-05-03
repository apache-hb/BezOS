#pragma once

#include <stddef.h>

namespace pv {
    void *SharedObjectMalloc(size_t size);
    void *SharedObjectAlignedAlloc(size_t align, size_t size);
    void SharedObjectFree(void *ptr);
}

#define PVTEST_SHARED_OBJECT(name) \
    static void *operator new(size_t size) { return pv::SharedObjectAlignedAlloc(alignof(name), size); } \
    static void *operator new[](size_t size) { return pv::SharedObjectAlignedAlloc(alignof(name), size); } \
    static void operator delete(void *ptr) noexcept { pv::SharedObjectFree(ptr); } \
    static void operator delete[](void *ptr) noexcept { pv::SharedObjectFree(ptr); }
