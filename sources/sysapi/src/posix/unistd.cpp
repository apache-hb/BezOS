#include <posix/unistd.h>
#include <posix/errno.h>

long sysconf(int param) noexcept {
    switch (param) {
    case _SC_PAGESIZE: return 0x1000;
    default:
        errno = EINVAL;
        return -1;
    }
}

int getopt(int, char *const[], const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

ssize_t read(int, void *, size_t) noexcept {
    errno = ENOSYS;
    return -1;
}

ssize_t write(int, const void *, size_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int isatty(int) noexcept {
    errno = ENOSYS;
    return -1;
}

int access(const char *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

uid_t getuid(void) noexcept {
    errno = ENOSYS;
    return -1;
}

uid_t geteuid(void) noexcept {
    errno = ENOSYS;
    return -1;
}

gid_t getgid(void) noexcept {
    errno = ENOSYS;
    return -1;
}

gid_t getegid(void) noexcept {
    errno = ENOSYS;
    return -1;
}

int setuid(uid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int seteuid(uid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int setgid(gid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int setegid(gid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int setpgrp(pid_t, pid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int getpgrp(pid_t) noexcept {
    errno = ENOSYS;
    return -1;
}

pid_t fork(void) noexcept {
    errno = ENOSYS;
    return -1;
}

char *getcwd(char *, size_t) noexcept {
    errno = ENOSYS;
    return nullptr;
}

extern "C" char *optarg = nullptr;
extern "C" int opterr = 0;
extern "C" int optind = 1;
extern "C" int optopt = 0;
