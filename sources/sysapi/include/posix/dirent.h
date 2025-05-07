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

extern int closedir(DIR *);
extern int dirfd(DIR *);
extern DIR *opendir(const char *);
extern struct dirent *readdir(DIR *);
extern void rewinddir(DIR *);

extern void seekdir(DIR *, long);
extern long telldir(DIR *);

BZP_API_END

#endif /* BEZOS_POSIX_DIRENT_H */
