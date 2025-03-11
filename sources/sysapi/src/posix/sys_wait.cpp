#include <posix/sys/wait.h>

#include "private.hpp"

pid_t wait(int *) noexcept {
    Unimplemented();
    return -1;
}

pid_t waitpid(pid_t, int *, int) noexcept {
    Unimplemented();
    return -1;
}
