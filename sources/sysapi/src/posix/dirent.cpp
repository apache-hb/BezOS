#include <posix/dirent.h>

#include "private.hpp"

int closedir(DIR *) noexcept {
    Unimplemented();
    return -1;
}

int dirfd(DIR *) noexcept {
    Unimplemented();
    return -1;
}

DIR *opendir(const char *) noexcept {
    Unimplemented();
    return nullptr;
}

struct dirent *readdir(DIR *) noexcept {
    Unimplemented();
    return nullptr;
}

void rewinddir(DIR *) noexcept {
    Unimplemented();
}


void seekdir(DIR *, long) noexcept {
    Unimplemented();
}

long telldir(DIR *) noexcept {
    Unimplemented();
    return -1;
}
