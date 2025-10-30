#include <posix/sys/wait.h>

#include "private.hpp"

pid_t wait(int *) {
    Unimplemented();
    return -1;
}

pid_t waitpid(pid_t, int *, int) {
    Unimplemented();
    return -1;
}
