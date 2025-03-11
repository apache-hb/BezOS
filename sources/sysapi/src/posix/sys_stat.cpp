#include <posix/sys/stat.h>

#include "private.hpp"

int fstat(int, struct stat *) noexcept {
    Unimplemented();
    return -1;
}

int stat(const char*, struct stat*) noexcept {
    Unimplemented();
    return -1;
}

int mkdir(const char*, mode_t) noexcept {
    Unimplemented();
    return -1;
}

int mkfifo(const char *, mode_t) noexcept {
    Unimplemented();
    return -1;
}

mode_t umask(mode_t) noexcept {
    Unimplemented();
    return 0;
}
