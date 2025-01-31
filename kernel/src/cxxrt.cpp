#include "log.hpp"
#include "panic.hpp"
#include "util/memory.hpp"

extern "C" void __cxa_pure_virtual() {
    KM_PANIC("Pure virtual function called.");
}

const std::nothrow_t std::nothrow{};

extern "C" void *malloc(size_t size);
extern "C" void free(void *ptr);

void *operator new(size_t size) {
    if (void *ptr = malloc(size)) {
        return ptr;
    }

    KmDebugMessage("[CRT] Allocation of ", sm::bytes(size), " failed.\n");
    KM_PANIC("Failed to allocate memory.");
}

void *operator new[](size_t size) {
    return operator new(size);
}

void *operator new[](size_t size, std::nothrow_t const&) noexcept {
    return malloc(size);
}

void operator delete(void *ptr, size_t _) noexcept {
    free(ptr);
}

void operator delete(void* ptr) noexcept {
    operator delete(ptr, 0zu);
}

void operator delete[](void *ptr) noexcept {
    operator delete(ptr);
}
