#include <posix/signal.h>

#include <posix/errno.h>

extern void (*signal(int, void (*)(int) noexcept))(int) noexcept {
    errno = ENOSYS;
    return nullptr;
}

int kill(pid_t, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigprocmask(int, const sigset_t *, sigset_t *) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigemptyset(sigset_t *) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigdelset(sigset_t *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigismember(const sigset_t *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigaddset(sigset_t *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigaction(int, const struct sigaction *, struct sigaction *) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigsuspend(const sigset_t *) noexcept {
    errno = ENOSYS;
    return -1;
}

int sigfillset(sigset_t *) noexcept {
    errno = ENOSYS;
    return -1;
}
