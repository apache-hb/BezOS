#ifndef BEZOS_POSIX_STROPTS_H
#define BEZOS_POSIX_STROPTS_H 1

#include <detail/cxx.h>

#include <asm/termbits.h> /* TIOC* definitions */

BZP_API_BEGIN

extern int ioctl(int, int, ...);

BZP_API_END

#endif /* BEZOS_POSIX_STROPTS_H */
