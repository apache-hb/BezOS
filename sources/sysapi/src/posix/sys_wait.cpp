#include <posix/sys/wait.h>

#include <posix/errno.h>

pid_t wait(int *) noexcept {
    errno = ENOSYS;
    return -1;
}

pid_t waitpid(pid_t, int *, int) noexcept {
    errno = ENOSYS;
    return -1;
}
