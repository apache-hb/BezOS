#include <posix/errno.h>
#include <posix/string.h>

static thread_local int gPosixErrno = 0;

int *OsImplPosixErrnoAddress(void) noexcept {
    return &gPosixErrno;
}

char *strerror(int err) noexcept {
    switch (err) {
    case EPERM: return (char*)"Operation not permitted";
    case ENOENT: return (char*)"No such file or directory";
    case ESRCH: return (char*)"No such process";

    case EDOM: return (char*)"Math argument out of domain of func";
    case ERANGE: return (char*)"Math result not representable";
    case EILSEQ: return (char*)"Illegal byte sequence";

    default: return (char*)"Unknown error";
    }
}
