#include <posix/string.h>

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

void *memset(void *ptr, int v, size_t size) {
    unsigned char *p = static_cast<unsigned char *>(ptr);
    for (size_t i = 0; i < size; i++) {
        p[i] = static_cast<unsigned char>(v);
    }
    return ptr;
}

void *memcpy(void *dst, const void *src, size_t size) {
    unsigned char *d = static_cast<unsigned char *>(dst);
    const unsigned char *s = static_cast<const unsigned char *>(src);
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t size) {
    unsigned char *d = static_cast<unsigned char *>(dst);
    const unsigned char *s = static_cast<const unsigned char *>(src);
    if (d < s) {
        for (size_t i = 0; i < size; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = size; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dst;
}

int strcmp(const char *lhs, const char *rhs) {
    while (*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *lhs - *rhs;
}
