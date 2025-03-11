#include <posix/sys/resource.h>

#include "private.hpp"

int getpriority(int, id_t) noexcept {
    Unimplemented();
    return -1;
}

int getrlimit(int, struct rlimit*) noexcept {
    Unimplemented();
    return -1;
}

int getrusage(int, struct rusage*) noexcept {
    Unimplemented();
    return -1;
}

int setpriority(int, id_t, int) noexcept {
    Unimplemented();
    return -1;
}

int setrlimit(int, const struct rlimit*) noexcept {
    Unimplemented();
    return -1;
}
