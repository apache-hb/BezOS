#include <posix/dlfcn.h>

#include "private.hpp"

int dlclose(void *) {
    Unimplemented();
    return -1;
}

char *dlerror(void) {
    Unimplemented();
    return nullptr;
}

void *dlopen(const char *, int) {
    Unimplemented();
    return nullptr;
}

void *dlsym(void *, const char *) {
    Unimplemented();
    return nullptr;
}
