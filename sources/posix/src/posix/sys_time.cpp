#include <posix/sys/time.h>

#include "private.hpp"

int gettimeofday(struct timeval *, void *) {
    Unimplemented();
    return -1;
}
