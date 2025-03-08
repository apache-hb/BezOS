#ifndef BEZOS_POSIX_UNISTD_H
#define BEZOS_POSIX_UNISTD_H 1

#include <detail/cxx.h>
#include <detail/attributes.h>
#include <detail/id.h>
#include <detail/size.h>
#include <detail/node.h>

#include <stddef.h>

BZP_API_BEGIN

#define _SC_PAGESIZE 0x100
#define _SC_OPEN_MAX 0x101

BZP_NORETURN
extern void _exit(int) BZP_NOEXCEPT;

extern int execve(const char *, char *const[], char *const[]) BZP_NOEXCEPT;

extern int dup(int) BZP_NOEXCEPT;

extern int close(int) BZP_NOEXCEPT;

extern int pipe(int[2]) BZP_NOEXCEPT;

extern int isatty(int) BZP_NOEXCEPT;

extern int chdir(const char *) BZP_NOEXCEPT;

extern int gethostname(char *, size_t) BZP_NOEXCEPT;

extern char *ttyname(int) BZP_NOEXCEPT;

extern int unlink(const char *) BZP_NOEXCEPT;

extern int link(const char *, const char *) BZP_NOEXCEPT;

extern pid_t getpid(void) BZP_NOEXCEPT;

extern pid_t getppid(void) BZP_NOEXCEPT;

extern uid_t getuid(void) BZP_NOEXCEPT;

extern uid_t geteuid(void) BZP_NOEXCEPT;

extern gid_t getgid(void) BZP_NOEXCEPT;

extern gid_t getegid(void) BZP_NOEXCEPT;

extern int setuid(uid_t) BZP_NOEXCEPT;

extern int seteuid(uid_t) BZP_NOEXCEPT;

extern int setgid(gid_t) BZP_NOEXCEPT;

extern int setegid(gid_t) BZP_NOEXCEPT;

extern int setpgrp(pid_t, pid_t) BZP_NOEXCEPT;
extern int getpgrp(pid_t) BZP_NOEXCEPT;

extern unsigned sleep(unsigned) BZP_NOEXCEPT;

extern unsigned alarm(unsigned) BZP_NOEXCEPT;

extern off_t lseek(int, off_t, int) BZP_NOEXCEPT;

extern ssize_t read(int, void *, size_t) BZP_NOEXCEPT;

extern ssize_t write(int, const void *, size_t) BZP_NOEXCEPT;

extern int access(const char *, int) BZP_NOEXCEPT;

extern pid_t fork(void) BZP_NOEXCEPT;

extern long sysconf(int) BZP_NOEXCEPT;

extern size_t confstr(int, char *, size_t) BZP_NOEXCEPT;

extern int getopt(int, char *const[], const char *) BZP_NOEXCEPT;

extern char *getcwd(char *, size_t) BZP_NOEXCEPT;

extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

BZP_API_END

#endif /* BEZOS_POSIX_UNISTD_H */
