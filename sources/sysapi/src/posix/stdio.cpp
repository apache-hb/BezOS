#include <posix/stdio.h>
#include <posix/errno.h>

struct OsImplPosixFile {

};

int fclose(FILE *file) {
    errno = ENOSYS;
    return -1;
}

FILE *fopen(const char *filename, const char *mode) {
    errno = ENOSYS;
    return nullptr;
}
