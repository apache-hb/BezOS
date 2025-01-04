#include <string.h>
#include <stdint.h>

#include "crt.hpp"

static void CopySmall(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

static size_t CopyQword(uint64_t *dst, const uint64_t *src, size_t n) {
    while (n > sizeof(uint64_t)) {
        *dst++ = *src++;
        n -= sizeof(uint64_t);
    }

    return n;
}

static void CopyLarge(uint8_t *dst, const uint8_t *src, size_t n) {
    // copy enough to align the destination to 8 bytes
    size_t align = (uintptr_t)dst % sizeof(uint64_t);
    CopySmall(dst, src, align);
    dst += align;
    src += align;
    n -= align;

    // copy 8 bytes at a time
    size_t remaining = CopyQword((uint64_t *)dst, (const uint64_t *)src, n);

    // copy the remaining bytes
    dst += n - remaining;
    src += n - remaining;

    CopySmall(dst, src, remaining);
}

void KmMemoryCopy(void *to, const void *from, size_t n) {
    uint8_t *dst = (uint8_t *)to;
    const uint8_t *src = (const uint8_t *)from;

    if (n > (sizeof(uint64_t) * 2)) {
        CopyLarge(dst, src, n);
    } else {
        CopySmall(dst, src, n);
    }
}


static void SetSmall(uint8_t *dst, uint8_t value, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = value;
    }
}

static void SetLarge(uint8_t *dst, uint8_t value, size_t n) {
    // align the destination to 8 bytes
    size_t align = (uintptr_t)dst % sizeof(uint64_t);
    SetSmall(dst, value, align);
    dst += align;
    n -= align;

    // set 8 bytes at a time
    uint64_t v64 = uint64_t(value) * 0x0101010101010101;
    while (n > sizeof(uint64_t)) {
        *reinterpret_cast<uint64_t*>(dst) = v64;
        dst += sizeof(uint64_t);
        n -= sizeof(uint64_t);
    }

    // set the remaining bytes
    SetSmall(dst, value, n);
}

void KmMemorySet(void *dst, uint8_t value, size_t size) {
    uint8_t *p = (uint8_t *)dst;

    if (size > (sizeof(uint64_t) * 2)) {
        SetLarge(p, value, size);
    } else {
        SetSmall(p, value, size);
    }
}
