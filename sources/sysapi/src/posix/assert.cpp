#include <posix/assert.h>

#include <posix/stdlib.h>
#include <posix/stdio.h>

static void PosixDefaultAssert(const char *expr, const char *file, unsigned line) {
    fprintf(stderr, "POSIX assertion failed: %s, file %s, line %u\n", expr, file, line);
    abort();
}

static void (*gAssertHandler)(const char *, const char *, unsigned) = PosixDefaultAssert;

void OsImplPosixAssert(const char *expr, const char *file, unsigned line) {
    gAssertHandler(expr, file, line);
}

void OsImplPosixInstall(void(*handler)(const char *, const char *, unsigned)) {
    gAssertHandler = handler;
}
