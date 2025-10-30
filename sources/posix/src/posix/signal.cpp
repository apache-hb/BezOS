#include <posix/signal.h>

#include "private.hpp"

extern void (*signal(int, void (*)(int)))(int) {
    Unimplemented();
    return nullptr;
}

int kill(pid_t, int) {
    Unimplemented();
    return -1;
}

int sigprocmask(int, const sigset_t *, sigset_t *) {
    Unimplemented();
    return -1;
}

int sigemptyset(sigset_t *) {
    Unimplemented();
    return -1;
}

int sigdelset(sigset_t *, int) {
    Unimplemented();
    return -1;
}

int sigismember(const sigset_t *, int) {
    Unimplemented();
    return -1;
}

int sigaddset(sigset_t *, int) {
    Unimplemented();
    return -1;
}

int sigaction(int, const struct sigaction *, struct sigaction *) {
    Unimplemented();
    return -1;
}

int sigsuspend(const sigset_t *) {
    Unimplemented();
    return -1;
}

int sigfillset(sigset_t *) {
    Unimplemented();
    return -1;
}
