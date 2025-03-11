#include <posix/sys/mman.h>

#include "private.hpp"

void *mmap(void *, size_t, int, int, int, off_t) noexcept {
    Unimplemented();
    return MAP_FAILED;
}

int munmap(void *, size_t) noexcept {
    Unimplemented();
    return -1;
}

int posix_madvise(void *, size_t, int) noexcept {
    Unimplemented();
    return -1;
}
