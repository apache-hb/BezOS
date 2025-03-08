#include <posix/stropts.h>

#include <posix/errno.h>

int ioctl(int, int, ...) noexcept {
    errno = ENOSYS;
    return -1;
}
