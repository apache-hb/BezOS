#ifndef BEZOS_POSIX_ERRNO_H
#define BEZOS_POSIX_ERRNO_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int *OsImplPosixErrnoAddress(void);
#define errno (*OsImplPosixErrnoAddress())

/**
 * These are intentially different constants than other posix implementations.
 * This is to ensure that ported programs actually follow the posix spec.
 */

#define EPERM  0x1000 /* Operation not permitted */
#define ENOENT 0x1001 /* No such file or directory */
#define ESRCH  0x1002 /* No such process */

#define ENOSYS 0x1003 /* Function not implemented */

/* Required by ISO C */
#define EDOM   0x1010 /* Math argument out of domain of func */
#define ERANGE 0x1011 /* Math result not representable */
#define EILSEQ 0x1012 /* Illegal byte sequence */

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_ERRNO_H */
