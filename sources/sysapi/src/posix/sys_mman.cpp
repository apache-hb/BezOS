#include <posix/sys/mman.h>
#include <posix/errno.h>

void *mmap(void *, size_t, int, int, int, off_t) noexcept {
    errno = ENOSYS;
    return MAP_FAILED;
}

int munmap(void *, size_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int posix_madvise(void *, size_t, int) noexcept {
    errno = ENOSYS;
    return -1;
}
