#ifndef BEZOS_POSIX_UNISTD_H
#define BEZOS_POSIX_UNISTD_H 1

#include <detail/id.h>
#include <detail/size.h>
#include <detail/node.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _SC_PAGESIZE 0x100

extern int execve(const char *, char *const[], char *const[]);

extern int dup(int);

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

extern unsigned sleep(unsigned);

extern unsigned alarm(unsigned);

extern off_t lseek(int, off_t, int);

extern ssize_t read(int, void *, size_t);

extern ssize_t write(int, const void *, size_t);

extern int access(const char *, int);

extern long sysconf(int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_UNISTD_H */
