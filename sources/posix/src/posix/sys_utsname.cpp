#include <posix/sys/utsname.h>

#include <posix/string.h>

#include "private.hpp"

int uname(struct utsname *uname) {
    strncpy(uname->sysname, "BezOS", sizeof(uname->sysname));
    strncpy(uname->nodename, "localhost", sizeof(uname->nodename));
    strncpy(uname->release, "0.0.1", sizeof(uname->release));
    strncpy(uname->version, "ides", sizeof(uname->version));
    strncpy(uname->machine, "amd64", sizeof(uname->machine));
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX uname");
    return -1;
}
