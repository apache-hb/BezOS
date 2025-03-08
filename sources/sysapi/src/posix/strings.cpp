#include <posix/strings.h>

#include <posix/ctype.h>

int strcasecmp(const char *lhs, const char *rhs) noexcept {
    while (*lhs && *rhs) {
        int l = tolower(*lhs++);
        int r = tolower(*rhs++);

        if (l != r) {
            return l - r;
        }
    }

    return tolower(*lhs) - tolower(*rhs);
}

int strncasecmp(const char *lhs, const char *rhs, size_t n) noexcept {
    for (size_t i = 0; i < n; i++) {
        char l = tolower(lhs[i]);
        char r = tolower(rhs[i]);

        if (l != r || l == '\0' || r == '\0') {
            return l - r;
        }
    }

    return 0;
}
