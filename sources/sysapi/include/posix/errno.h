#ifndef BEZOS_POSIX_ERRNO_H
#define BEZOS_POSIX_ERRNO_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

extern int *OsImplPosixErrnoAddress(void) BZP_NOEXCEPT;
#define errno (*OsImplPosixErrnoAddress())

/**
 * These are intentially different constants than other posix implementations.
 * This is to ensure that ported programs actually follow the posix spec.
 */

#define EPERM 0x1 /* Operation not permitted */
#define ENOENT 0x2 /* No such file or directory */
#define ESRCH 0x3 /* No such process */

#define ENOSYS 0x4 /* Function not implemented */

/* Required by ISO C */
#define EDOM 0x5 /* Math argument out of domain of func */
#define ERANGE 0x6 /* Math result not representable */
#define EILSEQ 0x7 /* Illegal byte sequence */

#define EINTR 0x8 /* Interrupted system call */
#define EINVAL 0x9 /* Invalid argument */
#define ESPIPE 0xA /* Illegal seek */
#define EBADF 0xB /* Bad file number */
#define ENOEXEC 0xC /* Exec format error */
#define ENOMEM 0xD /* Out of memory */
#define E2BIG 0xE /* Argument list too long */
#define ENOTDIR 0xF /* Not a directory */
#define EIO 0x10 /* I/O error */
#define EIEIO 0x11 /* Computer bought the farm */
#define EACCES 0x12 /* Permission denied */
#define EEXIST 0x13 /* File exists */

BZP_API_END

#endif /* BEZOS_POSIX_ERRNO_H */
