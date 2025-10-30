#ifndef BEZOS_POSIX_SYS_SELECT_H
#define BEZOS_POSIX_SYS_SELECT_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fd_set fd_set;

extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_SELECT_H */
