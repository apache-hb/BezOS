#include <posix/sys/times.h>

#include <posix/errno.h>

clock_t times(struct tms *) {
    errno = ENOSYS;
    return -1;
}
