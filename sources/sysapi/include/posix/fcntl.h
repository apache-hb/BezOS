#ifndef BEZOS_POSIX_FCNTL_H
#define BEZOS_POSIX_FCNTL_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int open(const char*, int, ...);

extern int fcntl(int, int, ...);

extern int close(int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_FCNTL_H */
