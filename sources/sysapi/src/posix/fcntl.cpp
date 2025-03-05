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
