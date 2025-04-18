#ifndef BEZOS_POSIX_SYS_TIME_H
#define BEZOS_POSIX_SYS_TIME_H 1

#include <detail/time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int gettimeofday(struct timeval *, void *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_TIME_H */
