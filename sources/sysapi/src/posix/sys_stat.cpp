#include <posix/sys/stat.h>

#include <posix/errno.h>

int fstat(int, struct stat *) noexcept {
    errno = ENOSYS;
    return -1;
}

int stat(const char*, struct stat*) noexcept {
    errno = ENOSYS;
    return -1;
}
