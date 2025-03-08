#include <posix/time.h>

#include <posix/errno.h>

time_t time(time_t *) noexcept {
    errno = ENOSYS;
    return (time_t)(-1);
}

struct tm *localtime(const time_t *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

size_t strftime(char *, size_t, const char *, const struct tm *) noexcept {
    errno = ENOSYS;
    return 0;
}
