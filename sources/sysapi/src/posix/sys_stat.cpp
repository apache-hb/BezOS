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

int mkdir(const char*, mode_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int mkfifo(const char *, mode_t) noexcept {
    errno = ENOSYS;
    return -1;
}

mode_t umask(mode_t) noexcept {
    errno = ENOSYS;
    return 0;
}
