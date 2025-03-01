#include <posix/sys/mman.h>
#include <posix/errno.h>

void *mmap(void *, size_t, int, int, int, off_t) {
    errno = ENOSYS;
    return MAP_FAILED;
}

int munmap(void *, size_t) {
    errno = ENOSYS;
    return -1;
}

int posix_madvise(void *, size_t, int) {
    errno = ENOSYS;
    return -1;
}
