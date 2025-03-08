#include <posix/unistd.h>

#include <posix/errno.h>
#include <posix/stdlib.h>

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

int dup(int) noexcept {
    errno = ENOSYS;
    return -1;
}

int dup2(int, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int pipe(int[2]) noexcept {
    errno = ENOSYS;
    return -1;
}

int getopt(int, char *const[], const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

off_t lseek(int, off_t, int) noexcept {
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

char *ttyname(int) noexcept {
    errno = ENOSYS;
    return nullptr;
}

pid_t getpid(void) noexcept {
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

int chdir(const char *) noexcept {
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

char *getlogin(void) noexcept {
    errno = ENOSYS;
    return nullptr;
}

int getlogin_r(char *, size_t) noexcept {
    errno = ENOSYS;
    return -1;
}

int pause(void) noexcept {
    errno = ENOSYS;
    return -1;
}

int link(const char *, const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

int unlink(const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

unsigned sleep(unsigned) noexcept {
    errno = ENOSYS;
    return 0;
}

unsigned alarm(unsigned) noexcept {
    errno = ENOSYS;
    return 0;
}

int execve(const char *, char *const[], char *const[]) noexcept {
    errno = ENOSYS;
    return -1;
}

pid_t getppid(void) noexcept {
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

extern "C" char **environ = nullptr;
