#include <posix/string.h>

#include <posix/stdint.h>
#include <posix/stdlib.h>

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
size_t strnlen(const char *s, size_t n) {
    size_t len = 0;
    while (n-- && *s++) {
        len++;
    }
    return len;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
void *memset(void *ptr, int v, size_t size) {
    unsigned char *p = static_cast<unsigned char *>(ptr);
    for (size_t i = 0; i < size; i++) {
        p[i] = static_cast<unsigned char>(v);
    }
    return ptr;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
void *memcpy(void *dst, const void *src, size_t size) {
    unsigned char *d = static_cast<unsigned char *>(dst);
    const unsigned char *s = static_cast<const unsigned char *>(src);
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dst;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
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

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = static_cast<const unsigned char *>(s);
    for (size_t i = 0; i < n; i++) {
        if (p[i] == c) {
            return const_cast<unsigned char *>(p + i);
        }
    }

    return nullptr;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
int memcmp(const void *lhs, const void *rhs, size_t n) {
    const unsigned char *l = static_cast<const unsigned char *>(lhs);
    const unsigned char *r = static_cast<const unsigned char *>(rhs);
    for (size_t i = 0; i < n; i++) {
        if (l[i] != r[i]) {
            return l[i] - r[i];
        }
    }

    return 0;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
int strcmp(const char *lhs, const char *rhs) {
    while (*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *lhs - *rhs;
}

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
int strncmp(const char *lhs, const char *rhs, size_t n) {
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

char *strcpy(char *dst, const char *src) {
    while (*src) {
        *dst++ = *src++;
    }

    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];

        if (src[i] == '\0') {
            return dst + i;
        }
    }

    return (dst + n);
}

char *strcat(char *dst, const char *src) {
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

char *strncat(char *dst, const char *src, size_t n) {
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

char *strchr(const char *str, int c) {
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == c) {
            return const_cast<char *>(str + i);
        }
    }

    return nullptr;
}

char *strrchr(const char *str, int c) {
    const char *last = nullptr;
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == c) {
            last = str + i;
        }
    }

    return const_cast<char *>(last);
}

char *strstr(const char *haystack, const char *needle) {
    size_t sublen = strlen(needle);
    for (size_t i = 0; haystack[i]; i++) {
        if (strncmp(haystack + i, needle, sublen) == 0) {
            return const_cast<char *>(haystack + i);
        }
    }

    return nullptr;
}

__attribute__((__nothrow__, __nonnull__, __malloc__, __allocating__))
char *strdup(const char *str) {
    size_t len = strlen(str);
    if (char *result = (char*)malloc(len + 1)) {
        memcpy(result, str, len);
        result[len] = '\0';
        return result;
    }

    return nullptr;
}

__attribute__((__nothrow__, __nonnull__, __malloc__, __allocating__))
char *strndup(const char *str, size_t len) {
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

static constexpr bool ImplStringContains(const char *str, char c) {
    while (*str) {
        if (*str == c) {
            return true;
        }

        str++;
    }

    return false;
}

size_t strspn(const char *str, const char *accept) {
    size_t count = 0;
    while (*str && ImplStringContains(accept, *str)) {
        count++;
        str++;
    }

    return count;
}
