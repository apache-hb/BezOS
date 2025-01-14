#include <string.h>
#include <stdint.h>

#include "crt.hpp"
#include "panic.hpp"

extern "C" [[noreturn]] void abort() {
    KM_PANIC("abort() called");
}

extern "C" void *memcpy(void *dest, const void *source, size_t n) {
    KmMemoryCopy(dest, source, n);
    return dest;
}

extern "C" void *memset(void *dst, int value, size_t n) {
    KmMemorySet(dst, value, n);
    return dst;
}

extern "C" void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

extern "C" int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

extern "C" char *strcpy(char *dest, const char *src) {
    char *pdest = dest;
    const char *psrc = src;

    while (*psrc != '\0') {
        *pdest++ = *psrc++;
    }

    *pdest = '\0';

    return dest;
}
