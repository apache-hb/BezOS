#include <posix/fcntl.h>
#include <posix/errno.h>

int open(const char *path, int, ...) {
    errno = ENOSYS;
    return -1;
}

int close(int) {
    errno = ENOSYS;
    return -1;
}
