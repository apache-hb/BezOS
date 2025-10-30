#ifndef BEZOS_POSIX_SYS_TIMES_H
#define BEZOS_POSIX_SYS_TIMES_H 1

#include <detail/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};

extern clock_t times(struct tms *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_TIMES_H */
