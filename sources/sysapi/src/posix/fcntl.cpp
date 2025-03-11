#include <posix/fcntl.h>

#include "private.hpp"

int open(const char *, int, ...) noexcept {
    Unimplemented();
    return -1;
}

int close(int) noexcept {
    Unimplemented();
    return -1;
}

int fcntl(int, int, ...) noexcept {
    Unimplemented();
    return -1;
}

int rename(const char *, const char *) noexcept {
    Unimplemented();
    return -1;
}

int mkfifoat(int, const char *, mode_t) noexcept {
    Unimplemented();
    return -1;
}
