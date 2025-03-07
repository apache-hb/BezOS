#include <posix/stdlib.h>

#include <posix/errno.h>

int abs(int v) noexcept {
    return v < 0 ? -v : v;
}

long labs(long v) noexcept {
    return v < 0 ? -v : v;
}

long long llabs(long long v) noexcept {
    return v < 0 ? -v : v;
}

char *getenv(const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

extern int putenv(char *) noexcept {
    errno = ENOSYS;
    return -1;
}

extern int setenv(const char *, const char *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

extern int unsetenv(const char *) noexcept {
    errno = ENOSYS;
    return -1;
}
