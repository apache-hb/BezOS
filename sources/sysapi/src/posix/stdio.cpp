#include <posix/stdio.h>
#include <posix/errno.h>

struct OsImplPosixFile {

};

int fclose(FILE *) noexcept {
    errno = ENOSYS;
    return -1;
}

FILE *fopen(const char *, const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}
