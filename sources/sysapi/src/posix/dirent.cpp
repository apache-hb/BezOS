#include <posix/dirent.h>

#include <posix/errno.h>

int closedir(DIR *) noexcept {
    errno = ENOSYS;
    return -1;
}

int dirfd(DIR *) noexcept {
    errno = ENOSYS;
    return -1;
}

DIR *opendir(const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

struct dirent *readdir(DIR *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

void rewinddir(DIR *) noexcept {
    errno = ENOSYS;
}


void seekdir(DIR *, long) noexcept {
    errno = ENOSYS;
}

long telldir(DIR *) noexcept {
    errno = ENOSYS;
    return -1;
}
