#ifndef BEZOS_POSIX_SYS_UTSNAME_H
#define BEZOS_POSIX_SYS_UTSNAME_H 1

#include <detail/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
    char sysname[256];
    char nodename[256];
    char release[256];
    char version[256];
    char machine[256];
};

extern int uname(struct utsname *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_UTSNAME_H */
