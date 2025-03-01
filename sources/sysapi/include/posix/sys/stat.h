#ifndef BEZOS_POSIX_SYS_STAT_H
#define BEZOS_POSIX_SYS_STAT_H 1

#include <detail/time.h>
#include <detail/id.h>
#include <detail/node.h>

#ifdef __cplusplus
extern "C" {
#endif

#define S_ISBLK(x) (0)
#define S_ISCHR(x) (0)
#define S_ISDIR(x) (0)
#define S_ISFIFO(x) (0)
#define S_ISREG(x) (0)
#define S_ISLNK(x) (0)
#define S_ISSOCK(x) (0)

#define S_IRWXU (00700)
#define S_IRUSR (00400)
#define S_IWUSR (00200)
#define S_IXUSR (00100)
#define S_IRWXG (00070)
#define S_IRGRP (00040)
#define S_IWGRP (00020)
#define S_IXGRP (00010)
#define S_IRWXO (00007)
#define S_IROTH (00004)
#define S_IWOTH (00002)
#define S_IXOTH (00001)

#define S_ISUID (04000)
#define S_ISGID (02000)
#define S_ISVTX (01000)

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

    /* gnoided once again */
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec

    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

extern int fstat(int, struct stat*);
extern int stat(const char*, struct stat*);

extern int mkdir(const char*, mode_t);

extern mode_t umask(mode_t);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_STAT_H */
