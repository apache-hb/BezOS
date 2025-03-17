#include <string.h>
#include <stdint.h>

#include <exception>

#include "crt.hpp"
#include "log.hpp"
#include "panic.hpp"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT
#define STB_SPRINTF_NOUNALIGNED
#include "stb_sprintf.h"

extern "C" int printf(const char *fmt, ...) {
    char buffer[1024];
    va_list va;
    va_start(va, fmt);
    int result = STB_SPRINTF_DECORATE(vsnprintf)(buffer, sizeof(buffer), fmt, va);
    va_end(va);

    stdx::StringView message(buffer, buffer + result);
    KmDebugMessage(message);

    return result;
}

extern "C" void tlsf_default_assert(int cond, const char *msg) {
    if (!cond) {
        stdx::StringView message(msg, msg + std::char_traits<char>::length(msg));
        KmDebugMessage("TLSF assertion failed: ", message, "\n");
        KM_PANIC("TLSF assertion failed.");
    }
}

void std::terminate() noexcept {
    KM_PANIC("std::terminate() called");
}

extern "C" void abort() {
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

extern "C" void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;

    while (n-- > 0) {
        if (*p == (unsigned char)c) {
            return (void *)p;
        }

        p++;
    }

    return nullptr;
}

extern "C" int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *(unsigned char *)s1 - *(unsigned char *)s2;
        }

        if (*s1 == '\0') {
            return 0;
        }

        s1++;
        s2++;
    }

    return 0;
}

extern "C" int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *(unsigned char *)s1 - *(unsigned char *)s2;
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

extern "C" char *strncpy(char *dest, const char *src, size_t n) {
    char *pdest = dest;
    const char *psrc = src;

    while (n-- > 0 && *psrc != '\0') {
        *pdest++ = *psrc++;
    }

    while (n-- > 0) {
        *pdest++ = '\0';
    }

    return dest;
}

extern "C" size_t strlen(const char *str) {
    const char *back = str;

    while (*back++) { /* empty */ }

    return back - str - 1;
}

extern "C" size_t strnlen(const char *str, size_t maxlen) {
    const char *back = str;

    while (maxlen-- && *back++) { /* empty */ }

    return back - str - 1;
}
