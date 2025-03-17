#include <posix/unistd.h>

#include <posix/errno.h>
#include <posix/stdlib.h>
#include <posix/string.h>

#include "private.hpp"

long sysconf(int param) noexcept {
    switch (param) {
    case _SC_PAGESIZE: return 0x1000;
    default:
        errno = EINVAL;
        return -1;
    }
}

void _exit(int exitcode) noexcept {
    exit(exitcode);
}

int dup(int fd) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX dup: %d", fd);
    return -1;
}

int dup2(int, int) noexcept {
    Unimplemented();
    return -1;
}

int pipe(int[2]) noexcept {
    Unimplemented();
    return -1;
}

int getopt(int, char *const[], const char *) noexcept {
    Unimplemented();
    return -1;
}

off_t lseek(int, off_t, int) noexcept {
    Unimplemented();
    return -1;
}

ssize_t read(int, void *, size_t) noexcept {
    Unimplemented();
    return -1;
}

ssize_t write(int, const void *, size_t) noexcept {
    Unimplemented();
    return -1;
}

int isatty(int) noexcept {
    Unimplemented();
    return -1;
}

int access(const char *, int) noexcept {
    Unimplemented();
    return -1;
}

static char name[64] = "/dev/tty";

char *ttyname(int fd) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX ttyname: %d", fd);
    return name;
}

pid_t getpid(void) noexcept {
    Unimplemented();
    return -1;
}

uid_t getuid(void) noexcept {
    Unimplemented();
    return -1;
}

uid_t geteuid(void) noexcept {
    Unimplemented();
    return -1;
}

gid_t getgid(void) noexcept {
    Unimplemented();
    return -1;
}

gid_t getegid(void) noexcept {
    Unimplemented();
    return -1;
}

int chdir(const char *) noexcept {
    Unimplemented();
    return -1;
}

int setuid(uid_t) noexcept {
    Unimplemented();
    return -1;
}

int seteuid(uid_t) noexcept {
    Unimplemented();
    return -1;
}

int setgid(gid_t) noexcept {
    Unimplemented();
    return -1;
}

int setegid(gid_t) noexcept {
    Unimplemented();
    return -1;
}

int setpgrp(pid_t, pid_t) noexcept {
    Unimplemented();
    return -1;
}

int getpgrp(pid_t) noexcept {
    Unimplemented();
    return -1;
}

pid_t fork(void) noexcept {
    Unimplemented();
    return -1;
}

char *getlogin(void) noexcept {
    Unimplemented();
    return nullptr;
}

int getlogin_r(char *, size_t) noexcept {
    Unimplemented();
    return -1;
}

int pause(void) noexcept {
    Unimplemented();
    return -1;
}

int link(const char *, const char *) noexcept {
    Unimplemented();
    return -1;
}

int unlink(const char *) noexcept {
    Unimplemented();
    return -1;
}

unsigned sleep(unsigned) noexcept {
    Unimplemented();
    return 0;
}

unsigned alarm(unsigned) noexcept {
    Unimplemented();
    return 0;
}

int execve(const char *, char *const[], char *const[]) noexcept {
    Unimplemented();
    return -1;
}

pid_t getppid(void) noexcept {
    Unimplemented();
    return -1;
}

char *getcwd(char *dst, size_t n) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getcwd");
    char *cwd = getenv("CWD");

    if (cwd != nullptr) {
        strncpy(dst, cwd, n);
        return dst;
    }

    errno = ENOENT;
    return nullptr;
}

extern "C" char *optarg = nullptr;
extern "C" int opterr = 0;
extern "C" int optind = 1;
extern "C" int optopt = 0;

static const char *gEnvStrings[] = {
    "HOME=/Users/Guest",
    "PWD=/Users/Guest",
    nullptr
};

extern "C" char **environ = const_cast<char**>(gEnvStrings);
