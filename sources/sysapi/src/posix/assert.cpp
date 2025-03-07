#include <posix/assert.h>

#include <posix/stdlib.h>

void OsImplPosixAssert(const char *, const char *, unsigned) {
    /* TODO: print error message or call user handler */
    abort();
}
