#include <posix/sys/resource.h>

#include <posix/errno.h>

int getpriority(int, id_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int getrlimit(int, struct rlimit*) noexcept {
    errno = ENOSYS;
    return -1;
}

int getrusage(int, struct rusage*) noexcept {
    errno = ENOSYS;
    return -1;
}

int setpriority(int, id_t, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int setrlimit(int, const struct rlimit*) noexcept {
    errno = ENOSYS;
    return -1;
}
