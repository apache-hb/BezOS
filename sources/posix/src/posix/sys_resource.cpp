#include <posix/sys/resource.h>

#include "private.hpp"

int getpriority(int, id_t) {
    Unimplemented();
    return -1;
}

int getrlimit(int lim, struct rlimit *rlim) {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getrlimit: %d", lim);
    return -1;
}

int getrusage(int lim, struct rusage *ruse) {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getrusage: %d", lim);
    return -1;
}

int setpriority(int, id_t, int) {
    Unimplemented();
    return -1;
}

int setrlimit(int, const struct rlimit*) {
    Unimplemented();
    return -1;
}
