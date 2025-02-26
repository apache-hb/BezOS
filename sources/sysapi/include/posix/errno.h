#ifndef BEZOS_POSIX_ERRNO_H
#define BEZOS_POSIX_ERRNO_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int *OsImplPosixErrnoAddress(void);
#define errno (*OsImplPosixErrnoAddress())

#define EPERM  1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH  3 /* No such process */

/* Required by ISO C */
#define EDOM  33 /* Math argument out of domain of func */
#define ERANGE 34 /* Math result not representable */
#define EILSEQ 84 /* Illegal byte sequence */

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_ERRNO_H */
