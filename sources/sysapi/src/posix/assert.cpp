#include <posix/assert.h>

#include <posix/stdlib.h>
#include <posix/stdio.h>

#include "private.hpp"

static void PosixDefaultAssert(const char *expr, const char *file, unsigned line) {
    OsDebugMessage(eOsLogError, "POSIX assertion failed");
    // DebugLog(eOsLogError, "POSIX assertion failed: %s, file %s, line %u\n", expr, file, line);
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
