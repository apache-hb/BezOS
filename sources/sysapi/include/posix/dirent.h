#ifndef BEZOS_POSIX_DIRENT_H
#define BEZOS_POSIX_DIRENT_H 1

#include <detail/cxx.h>
#include <detail/node.h>

#include <limits.h>

BZP_API_BEGIN

struct OsImplPosixDir;

typedef struct OsPosixImplDir DIR;

struct dirent {
    ino_t d_ino;
    char d_name[NAME_MAX + 1];
};

extern int closedir(DIR *) BZP_NOEXCEPT;
extern int dirfd(DIR *) BZP_NOEXCEPT;
extern DIR *opendir(const char *) BZP_NOEXCEPT;
extern struct dirent *readdir(DIR *) BZP_NOEXCEPT;
extern void rewinddir(DIR *) BZP_NOEXCEPT;

extern void seekdir(DIR *, long) BZP_NOEXCEPT;
extern long telldir(DIR *) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_DIRENT_H */
