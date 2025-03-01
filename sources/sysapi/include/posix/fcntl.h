#ifndef BEZOS_POSIX_FCNTL_H
#define BEZOS_POSIX_FCNTL_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define F_SETFD 0x1
#define F_GETFD 0x2
#define F_SETFL 0x3
#define F_GETFL 0x4
#define F_DUPFD 0x5

#define F_OK (1 << 0)
#define W_OK (1 << 1)
#define R_OK (1 << 2)
#define X_OK (1 << 3)

#define O_CREAT (1 << 0)
#define O_TRUNC (1 << 1)
#define O_WRONLY (1 << 2)
#define O_RDONLY (1 << 3)
#define O_APPEND (1 << 4)
#define O_EXCL (1 << 5)
#define O_RDWR (O_RDONLY | O_WRONLY)

extern int open(const char *, int, ...);

extern int fcntl(int, int, ...);

extern int close(int);

extern int rename(const char *, const char *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_FCNTL_H */
