#include <posix/assert.h>

#include <posix/stdlib.h>
#include <posix/stdio.h>

#include "private.hpp"

static void PosixDefaultAssert(const char *expr, const char *file, unsigned line) {
    OsDebugMessage(eOsLogError, "POSIX assertion failed");
    char buffer[256];
    size_t len = snprintf(buffer, sizeof(buffer), "POSIX assertion failed: %s, file %s, line %u\n", expr, file, line);
    OsDebugMessageInfo messageInfo = {
        .Front = buffer,
        .Back = buffer + len,
        .Info = eOsLogError,
    };
    OsDebugMessage(messageInfo);
    abort();
}

static void (*gAssertHandler)(const char *, const char *, unsigned) = PosixDefaultAssert;

void OsImplPosixAssert(const char *expr, const char *file, unsigned line) {
    DebugLog(eOsLogError, "POSIX assertion failed\n");
    gAssertHandler(expr, file, line);
}

void OsImplPosixInstall(void(*handler)(const char *, const char *, unsigned)) {
    gAssertHandler = handler;
}
