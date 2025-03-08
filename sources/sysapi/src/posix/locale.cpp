#include <posix/locale.h>

#include <posix/errno.h>

char *setlocale(int, const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}
