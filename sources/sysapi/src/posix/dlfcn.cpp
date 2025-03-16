#include <posix/dlfcn.h>

#include "private.hpp"

int dlclose(void *) noexcept {
    Unimplemented();
    return -1;
}

char *dlerror(void) noexcept {
    Unimplemented();
    return nullptr;
}

void *dlopen(const char *, int) noexcept {
    Unimplemented();
    return nullptr;
}

void *dlsym(void *, const char *) noexcept {
    Unimplemented();
    return nullptr;
}
