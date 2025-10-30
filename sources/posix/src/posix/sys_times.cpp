#include <posix/sys/times.h>

#include "private.hpp"

clock_t times(struct tms *) {
    Unimplemented();
    return -1;
}
