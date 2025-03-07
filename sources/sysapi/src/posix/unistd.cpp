#include <posix/unistd.h>
#include <posix/errno.h>

long sysconf(int param) noexcept {
    switch (param) {
    case _SC_PAGESIZE: return 0x1000;
    default:
        errno = EINVAL;
        return -1;
    }
}

int getopt(int, char *const[], const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

char *getcwd(char *, size_t) noexcept {
    errno = ENOSYS;
    return nullptr;
}

extern "C" char *optarg = nullptr;
extern "C" int opterr = 0;
extern "C" int optind = 1;
extern "C" int optopt = 0;
