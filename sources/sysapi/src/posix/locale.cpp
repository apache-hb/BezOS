#include <posix/locale.h>

#include "private.hpp"

char *setlocale(int, const char *) noexcept {
    Unimplemented();
    return nullptr;
}
