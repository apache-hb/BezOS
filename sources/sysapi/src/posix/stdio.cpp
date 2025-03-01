#include <posix/stdio.h>
#include <posix/errno.h>

struct OsImplPosixFile {

};

int fclose(FILE *) {
    errno = ENOSYS;
    return -1;
}

FILE *fopen(const char *, const char *) {
    errno = ENOSYS;
    return nullptr;
}
