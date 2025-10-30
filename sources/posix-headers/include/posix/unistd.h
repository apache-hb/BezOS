#ifndef BEZOS_POSIX_UNISTD_H
#define BEZOS_POSIX_UNISTD_H 1

#include <detail/cxx.h>
#include <detail/attributes.h>
#include <detail/id.h>
#include <detail/size.h>
#include <detail/node.h>

#include <stddef.h>
#include <features.h>

BZP_API_BEGIN

#define _SC_PAGESIZE 0x100
#define _SC_OPEN_MAX 0x101

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

__attribute__((__nothrow__, __noreturn__))
extern void _exit(int);

extern int execve(const char *, char *const[], char *const[]);

extern int dup(int);

extern int dup2(int, int);

extern int close(int);

extern int pipe(int[2]);

extern int isatty(int);

extern int chdir(const char *);

extern int gethostname(char *, size_t);

extern char *ttyname(int);

extern int unlink(const char *);

extern int link(const char *, const char *);

extern pid_t getpid(void);

extern pid_t getppid(void);

extern uid_t getuid(void);

extern uid_t geteuid(void);

extern gid_t getgid(void);

extern gid_t getegid(void);

extern int setuid(uid_t);

extern int seteuid(uid_t);

extern int setgid(gid_t);

extern int setegid(gid_t);

#if JEFF_OPTION_BSD

extern int setpgrp(pid_t, pid_t);

extern int getpgrp(pid_t);

#endif

extern unsigned sleep(unsigned);

extern unsigned alarm(unsigned);

extern off_t lseek(int, off_t, int);

extern ssize_t read(int, void *, size_t);

extern ssize_t write(int, const void *, size_t);

extern int access(const char *, int);

extern pid_t fork(void);

extern long sysconf(int);

extern size_t confstr(int, char *, size_t);

extern char *getlogin(void);

extern int getlogin_r(char *, size_t);

extern int pause(void);

extern int getopt(int, char *const[], const char *);

extern char *getcwd(char *, size_t);

extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

extern char **environ;

BZP_API_END

#endif /* BEZOS_POSIX_UNISTD_H */
