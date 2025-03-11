#include <posix/stropts.h>

#include "private.hpp"

int ioctl(int, int, ...) noexcept {
    Unimplemented();
    return -1;
}
