#include "log.hpp"
#include "panic.hpp"
#include "util/memory.hpp"

#include "crt.hpp"

#include <new>

extern "C" void __cxa_pure_virtual() {
    KM_PANIC("Pure virtual function called.");
}

const std::nothrow_t std::nothrow{};

void* operator new(std::size_t size) {
    if (void* ptr = aligned_alloc(16, size)) {
        return ptr;
    }

    KmDebugMessage("[CRT] Allocation of ", sm::bytes(size), " failed.\n");
    KM_PANIC("Failed to allocate memory.");
}

void* operator new[](std::size_t size) {
    return operator new(size, std::align_val_t(16));
}

void operator delete(void* ptr) {
    free(ptr);
}

void operator delete[](void* ptr) {
    operator delete(ptr);
}

void operator delete(void* ptr, std::size_t) {
    operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t) {
    operator delete(ptr);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return operator new(size, std::align_val_t(alignof(std::max_align_t)));
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return operator new(size, std::nothrow);
}

void operator delete(void* ptr, const std::nothrow_t&) {
    operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) {
    operator delete(ptr, std::nothrow);
}

void* operator new(std::size_t size, std::align_val_t align) {
    return aligned_alloc(std::to_underlying(align), size);
}

void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return operator new(size, align);
}

void operator delete(void* ptr, std::align_val_t) {
    operator delete(ptr);
}

void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) {
    operator delete(ptr, std::nothrow);
}

void* operator new[](std::size_t size, std::align_val_t align) {
    return operator new(size, align);
}

void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return operator new(size, align, std::nothrow);
}

void operator delete[](void* ptr, std::align_val_t) {
    operator delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) {
    operator delete(ptr, std::nothrow);
}

void operator delete(void* ptr, std::size_t, std::align_val_t align) {
    operator delete(ptr, align);
}

void operator delete[](void* ptr, std::size_t size, std::align_val_t align) {
    operator delete(ptr, size, align);
}
