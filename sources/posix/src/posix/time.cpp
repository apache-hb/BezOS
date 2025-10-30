#include <posix/time.h>

#include "private.hpp"

time_t time(time_t *) {
    Unimplemented();
    return (time_t)(-1);
}

struct tm *localtime(const time_t *) {
    Unimplemented();
    return nullptr;
}

size_t strftime(char *, size_t, const char *, const struct tm *) {
    Unimplemented();
    return 0;
}
