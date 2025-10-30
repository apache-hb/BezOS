#ifndef BEZOS_POSIX_SIGNAL_H
#define BEZOS_POSIX_SIGNAL_H 1

#include <detail/id.h>
#include <detail/cxx.h>

BZP_API_BEGIN

#define SIG_IGN  ((void(*)(int))(0x1))
#define SIG_DFL  ((void(*)(int))(0x2))
#define SIG_ERR  ((void(*)(int))(0x3))
#define SIG_HOLD ((void(*)(int))(0x4))

#define SIG_BLOCK 0x1
#define SIG_UNBLOCK 0x2
#define SIG_SETMASK 0x3

#define SIGINT 0x1
#define SIGILL 0x2
#define SIGALRM 0x3
#define SIGTERM 0x4
#define SIGQUIT 0x5
#define SIGCHLD 0x6
#define SIGHUP 0x7
#define SIGKILL 0x8
#define SIGSTOP 0x9
#define SIGTSTP 0xA
#define SIGTTOU 0xB
#define SIGTTIN 0xC
#define SIGPIPE 0xD
#define SIGCONT 0xE

#define NSIG 0xF

typedef int sig_atomic_t;
typedef int sigset_t;

struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void(*sa_sigaction)(int, void *, void *);
};

extern void (*signal(int, void (*)(int)))(int);

extern int kill(pid_t, int);

extern int sigprocmask(int, const sigset_t *, sigset_t *);

extern int sigemptyset(sigset_t *);

extern int sigdelset(sigset_t *, int);

extern int sigismember(const sigset_t *, int);

extern int sigaddset(sigset_t *, int);

extern int sigaction(int, const struct sigaction *, struct sigaction *);

extern int sigsuspend(const sigset_t *);

extern int sigfillset(sigset_t *);

BZP_API_END

#endif /* BEZOS_POSIX_SIGNAL_H */
