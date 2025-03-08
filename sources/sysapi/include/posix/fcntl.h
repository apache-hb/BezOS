#ifndef BEZOS_POSIX_FCNTL_H
#define BEZOS_POSIX_FCNTL_H 1

#include <detail/cxx.h>
#include <detail/node.h>
#include <detail/id.h>

BZP_API_BEGIN

#define F_SETFD 0x1
#define F_GETFD 0x2
#define F_SETFL 0x3
#define F_GETFL 0x4
#define F_DUPFD 0x5
#define F_SETLK 0x6
#define F_SETLKW 0x7

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

#define F_RDLCK (1 << 0)
#define F_WRLCK (1 << 1)
#define F_UNLCK (1 << 2)

struct flock {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};

extern int open(const char *, int, ...) BZP_NOEXCEPT;

extern int fcntl(int, int, ...) BZP_NOEXCEPT;

extern int close(int) BZP_NOEXCEPT;

extern int rename(const char *, const char *) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_FCNTL_H */
