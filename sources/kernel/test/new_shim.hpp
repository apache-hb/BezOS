#pragma once

#include "common/util/util.hpp"

#include <new>
#include <stdlib.h>

enum AllocFlags {
    eNone = 0,
    eNoThrow = (1 << 0),
    eArray = (1 << 1),
};

UTIL_BITFLAGS(AllocFlags);

class IGlobalAllocator {
public:
    virtual ~IGlobalAllocator() = default;

    virtual void *operatorNew(size_t size, size_t align, AllocFlags flags) {
        if (void *ptr = aligned_alloc(align, size)) {
            return ptr;
        }

        if (bool(flags & eNoThrow)) {
            return nullptr;
        }

        throw std::bad_alloc();
    }

    virtual void operatorDelete(void *ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t align, [[maybe_unused]] AllocFlags flags) {
        if (ptr == nullptr) {
            return;
        }

        free(ptr);
    }
};

IGlobalAllocator *GetGlobalAllocator();
void SetGlobalAllocator(IGlobalAllocator *allocator);

#if defined(TEST_REPLACE_GLOBAL_ALLOCATOR)
static void *OperatorNew(size_t size, size_t align, AllocFlags flags) {
    return GetGlobalAllocator()->operatorNew(size, align, flags);
}

static void OperatorDelete(void *ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t align, [[maybe_unused]] AllocFlags flags) {
    GetGlobalAllocator()->operatorDelete(ptr, size, align, flags);
}

// operator new

void* operator new(std::size_t size) {
    return OperatorNew(size, alignof(std::max_align_t), eNone);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return OperatorNew(size, alignof(std::max_align_t), eNoThrow);
}

void* operator new(std::size_t size, std::align_val_t align) {
    return OperatorNew(size, std::to_underlying(align), eNone);
}

void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return OperatorNew(size, std::to_underlying(align), eNoThrow);
}

// operator new[]

void* operator new[](std::size_t size) {
    return OperatorNew(size, alignof(std::max_align_t), eArray);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return OperatorNew(size, alignof(std::max_align_t), eArray | eNoThrow);
}

void* operator new[](std::size_t size, std::align_val_t align) {
    return OperatorNew(size, std::to_underlying(align), eArray);
}

void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return OperatorNew(size, std::to_underlying(align), eArray | eNoThrow);
}

// operator delete

void operator delete(void* ptr) {
    OperatorDelete(ptr, 0, 0, eNone);
}

void operator delete(void* ptr, std::size_t size) {
    OperatorDelete(ptr, size, 0, eNone);
}

void operator delete(void* ptr, const std::nothrow_t&) {
    OperatorDelete(ptr, 0, 0, eNoThrow);
}

void operator delete(void* ptr, std::align_val_t align) {
    OperatorDelete(ptr, 0, std::to_underlying(align), eNone);
}

void operator delete(void* ptr, std::align_val_t align, const std::nothrow_t&) {
    OperatorDelete(ptr, 0, std::to_underlying(align), eNoThrow);
}

void operator delete(void* ptr, std::size_t size, std::align_val_t align) {
    OperatorDelete(ptr, size, std::to_underlying(align), eNone);
}

// operator delete[]

void operator delete[](void* ptr) {
    OperatorDelete(ptr, 0, 0, eArray);
}

void operator delete[](void* ptr, std::size_t size) {
    OperatorDelete(ptr, size, 0, eArray);
}

void operator delete[](void* ptr, const std::nothrow_t&) {
    OperatorDelete(ptr, 0, 0, eArray | eNoThrow);
}

void operator delete[](void* ptr, std::align_val_t align) {
    OperatorDelete(ptr, 0, std::to_underlying(align), eArray);
}

void operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t&) {
    OperatorDelete(ptr, 0, std::to_underlying(align), eArray | eNoThrow);
}

void operator delete[](void* ptr, std::size_t size, std::align_val_t align) {
    OperatorDelete(ptr, size, std::to_underlying(align), eArray);
}
#endif
