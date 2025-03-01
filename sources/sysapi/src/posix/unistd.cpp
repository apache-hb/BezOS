#include <posix/unistd.h>
#include <posix/errno.h>

long sysconf(int param) {
    switch (param) {
    case _SC_PAGESIZE: return 0x1000;
    default:
        errno = EINVAL;
        return -1;
    }
}
