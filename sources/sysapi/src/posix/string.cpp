#include <posix/string.h>

#include <posix/stdint.h>
#include <posix/stdlib.h>

size_t strlen(const char *s) noexcept {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

void *memset(void *ptr, int v, size_t size) noexcept {
    unsigned char *p = static_cast<unsigned char *>(ptr);
    for (size_t i = 0; i < size; i++) {
        p[i] = static_cast<unsigned char>(v);
    }
    return ptr;
}

void *memcpy(void *dst, const void *src, size_t size) noexcept {
    unsigned char *d = static_cast<unsigned char *>(dst);
    const unsigned char *s = static_cast<const unsigned char *>(src);
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t size) noexcept {
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

void *memchr(const void *s, int c, size_t n) noexcept {
    const unsigned char *p = static_cast<const unsigned char *>(s);
    for (size_t i = 0; i < n; i++) {
        if (p[i] == c) {
            return const_cast<unsigned char *>(p + i);
        }
    }

    return nullptr;
}

int memcmp(const void *lhs, const void *rhs, size_t n) noexcept {
    const unsigned char *l = static_cast<const unsigned char *>(lhs);
    const unsigned char *r = static_cast<const unsigned char *>(rhs);
    for (size_t i = 0; i < n; i++) {
        if (l[i] != r[i]) {
            return l[i] - r[i];
        }
    }

    return 0;
}

int strcmp(const char *lhs, const char *rhs) noexcept {
    while (*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *lhs - *rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t n) noexcept {
    for (size_t i = 0; i < n; i++) {
        char l = lhs[i];
        char r = rhs[i];

        if (l != r) {
            return l - r;
        }

        if (l == '\0' || r == '\0') {
            return 0;
        }
    }

    return 0;
}

char *strcpy(char *dst, const char *src) noexcept {
    while (*src) {
        *dst++ = *src++;
    }

    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) noexcept {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];

        if (src[i] == '\0') {
            return dst + i;
        }
    }

    return (dst + n);
}

char *strcat(char *dst, const char *src) noexcept {
    char *result = dst;

    while (*dst) {
        dst++;
    }

    while (*src) {
        *dst++ = *src++;
    }

    *dst = '\0';

    return result;
}

char *strncat(char *dst, const char *src, size_t n) noexcept {
    char *result = dst;

    while (*dst) {
        dst++;
    }

    for (size_t i = 0; i < n; i++) {
        *dst = src[i];

        if (src[i] == '\0') {
            return result;
        }

        dst++;
    }

    *dst = '\0';

    return result;
}

char *strchr(const char *str, int c) noexcept {
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == c) {
            return const_cast<char *>(str + i);
        }
    }

    return nullptr;
}

char *strrchr(const char *str, int c) noexcept {
    const char *last = nullptr;
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == c) {
            last = str + i;
        }
    }

    return const_cast<char *>(last);
}

char *strstr(const char *haystack, const char *needle) noexcept {
    size_t sublen = strlen(needle);
    for (size_t i = 0; haystack[i]; i++) {
        if (strncmp(haystack + i, needle, sublen) == 0) {
            return const_cast<char *>(haystack + i);
        }
    }

    return nullptr;
}

char *strdup(const char *str) noexcept {
    size_t len = strlen(str);
    if (char *result = (char*)malloc(len + 1)) {
        memcpy(result, str, len);
        result[len] = '\0';
        return result;
    }

    return nullptr;
}

char *strndup(const char *str, size_t len) noexcept {
    if (char *result = (char*)malloc(len + 1)) {
        size_t i;
        for (i = 0; i < len && str[i]; i++) {
            result[i] = str[i];
        }

        result[i] = '\0';

        return result;
    }

    return nullptr;
}

static constexpr bool ImplStringContains(const char *str, char c) noexcept {
    while (*str) {
        if (*str == c) {
            return true;
        }

        str++;
    }

    return false;
}

size_t strspn(const char *str, const char *accept) noexcept {
    size_t count = 0;
    while (*str && ImplStringContains(accept, *str)) {
        count++;
        str++;
    }

    return count;
}
