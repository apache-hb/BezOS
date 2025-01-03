#include <string.h>
#include <stdint.h>

#include "crt.hpp"

static size_t memcpy64(uint64_t *dst, const uint64_t *src, size_t n) {
    while (n > sizeof(uint64_t)) {
        *dst++ = *src++;
        n -= sizeof(uint64_t);
    }

    return n;
}

static void memcpyLarge(uint8_t *dst, const uint8_t *src, size_t n) {
    // copy enough to align the destination to 8 bytes
    while ((uintptr_t)dst % sizeof(uint64_t) != 0) {
        *dst++ = *src++;
        n--;
    }

    // copy 8 bytes at a time
    size_t remaining = memcpy64((uint64_t *)dst, (const uint64_t *)src, n);

    // copy the remaining bytes
    dst += n - remaining;
    src += n - remaining;

    while (remaining > 0) {
        *dst++ = *src++;
        remaining--;
    }
}

static void memcpySmall(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

void KmMemoryCopy(void *to, const void *from, size_t n) {
    uint8_t *dst = (uint8_t *)to;
    const uint8_t *src = (const uint8_t *)from;

    if (n > (sizeof(uint64_t) * 2)) {
        memcpyLarge(dst, src, n);
    } else {
        memcpySmall(dst, src, n);
    }
}
