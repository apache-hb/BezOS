#include <posix/sys/time.h>

#include <posix/errno.h>

int gettimeofday(struct timeval *, void *) {
    errno = ENOSYS;
    return -1;
}
