#ifndef BEZOS_POSIX_SYS_STAT_H
#define BEZOS_POSIX_SYS_STAT_H 1

#include <detail/time.h>
#include <detail/id.h>
#include <detail/node.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;

    uid_t st_uid;
    gid_t st_gid;

    dev_t st_rdev;
    off_t st_size;

    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;

    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

extern int fstat(int, struct stat*);
extern int stat(const char*, struct stat*);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_STAT_H */
