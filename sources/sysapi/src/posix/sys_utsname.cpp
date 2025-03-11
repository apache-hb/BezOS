#include <posix/sys/utsname.h>

#include "private.hpp"

int uname(struct utsname *) {
    Unimplemented();
    return -1;
}
