#include <posix/time.h>

#include <posix/errno.h>

time_t time(time_t *) {
    errno = ENOSYS;
    return (time_t)(-1);
}
