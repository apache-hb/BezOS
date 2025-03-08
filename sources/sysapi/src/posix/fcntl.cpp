#include <posix/fcntl.h>
#include <posix/errno.h>

int open(const char *, int, ...) noexcept {
    errno = ENOSYS;
    return -1;
}

int close(int) noexcept {
    errno = ENOSYS;
    return -1;
}

int fcntl(int, int, ...) noexcept {
    errno = ENOSYS;
    return -1;
}

int rename(const char *, const char *) noexcept {
    errno = ENOSYS;
    return -1;
}
