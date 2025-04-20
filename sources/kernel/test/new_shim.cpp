#include "new_shim.hpp"

TestAllocator *GetGlobalAllocator() {
    static TestAllocator sDefaultAllocator;
    return &sDefaultAllocator;
}

static void *OperatorNew(size_t size, size_t align, AllocFlags flags) {
    return GetGlobalAllocator()->operatorNew(size, align, flags);
}

static void OperatorDelete(void *ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t align, [[maybe_unused]] AllocFlags flags) {
    GetGlobalAllocator()->operatorDelete(ptr, size, align, flags);
}

// operator new

void* operator new(std::size_t size) {
    return OperatorNew(size, 1, eNone);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return OperatorNew(size, 1, eNoThrow);
}

void* operator new(std::size_t size, std::align_val_t align) {
    return OperatorNew(size, std::to_underlying(align), eNone);
}

void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return OperatorNew(size, std::to_underlying(align), eNoThrow);
}

// operator new[]

void* operator new[](std::size_t size) {
    return OperatorNew(size, 1, eArray);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return OperatorNew(size, 1, eArray | eNoThrow);
}

void* operator new[](std::size_t size, std::align_val_t align) {
    return OperatorNew(size, std::to_underlying(align), eArray);
}

void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return OperatorNew(size, std::to_underlying(align), eArray | eNoThrow);
}

// operator delete

void operator delete(void* ptr) noexcept {
    OperatorDelete(ptr, 0, 0, eNone);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    OperatorDelete(ptr, size, 0, eNone);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    OperatorDelete(ptr, 0, 0, eNoThrow);
}

void operator delete(void* ptr, std::align_val_t align) noexcept {
    OperatorDelete(ptr, 0, std::to_underlying(align), eNone);
}

void operator delete(void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept {
    OperatorDelete(ptr, 0, std::to_underlying(align), eNoThrow);
}

void operator delete(void* ptr, std::size_t size, std::align_val_t align) noexcept {
    OperatorDelete(ptr, size, std::to_underlying(align), eNone);
}

// operator delete[]

void operator delete[](void* ptr) noexcept {
    OperatorDelete(ptr, 0, 0, eArray);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    OperatorDelete(ptr, size, 0, eArray);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    OperatorDelete(ptr, 0, 0, eArray | eNoThrow);
}

void operator delete[](void* ptr, std::align_val_t align) noexcept {
    OperatorDelete(ptr, 0, std::to_underlying(align), eArray);
}

void operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept {
    OperatorDelete(ptr, 0, std::to_underlying(align), eArray | eNoThrow);
}

void operator delete[](void* ptr, std::size_t size, std::align_val_t align) noexcept {
    OperatorDelete(ptr, size, std::to_underlying(align), eArray);
}
