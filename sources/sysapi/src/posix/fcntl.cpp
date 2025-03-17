#include <posix/fcntl.h>

#include "private.hpp"

int open(const char *path, int mode, ...) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX open: '%s' %d", path, mode);
    return -1;
}

int close(int fd) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX close: %d", fd);
    return -1;
}

int fcntl(int fd, int ctrl, ...) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX fcntl: %d %d", fd, ctrl);
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
