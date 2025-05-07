#include <posix/stropts.h>

#include "private.hpp"

int ioctl(int, int, ...) {
    Unimplemented();
    return -1;
}
