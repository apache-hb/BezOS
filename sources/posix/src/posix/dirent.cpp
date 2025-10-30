#include <posix/dirent.h>

#include "private.hpp"

int closedir(DIR *) {
    Unimplemented();
    return -1;
}

int dirfd(DIR *) {
    Unimplemented();
    return -1;
}

DIR *opendir(const char *) {
    Unimplemented();
    return nullptr;
}

struct dirent *readdir(DIR *) {
    Unimplemented();
    return nullptr;
}

void rewinddir(DIR *) {
    Unimplemented();
}


void seekdir(DIR *, long) {
    Unimplemented();
}

long telldir(DIR *) {
    Unimplemented();
    return -1;
}
