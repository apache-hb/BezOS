#include <posix/sys/utsname.h>

#include <posix/errno.h>

int uname(struct utsname *) {
    errno = ENOSYS;
    return -1;
}
