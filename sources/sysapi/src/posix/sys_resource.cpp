#include <posix/sys/resource.h>

#include "private.hpp"

int getpriority(int, id_t) noexcept {
    Unimplemented();
    return -1;
}

int getrlimit(int lim, struct rlimit *rlim) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getrlimit: %d", lim);
    return -1;
}

int getrusage(int lim, struct rusage *ruse) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getrusage: %d", lim);
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
