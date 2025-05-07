#ifndef BEZOS_POSIX_TIME_H
#define BEZOS_POSIX_TIME_H 1

#include <detail/time.h>
#include <detail/size.h>
#include <detail/cxx.h>

BZP_API_BEGIN

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

extern time_t time(time_t *);

struct tm *localtime(const time_t *);

extern size_t strftime(char *, size_t, const char *, const struct tm *);

BZP_API_END

#endif /* BEZOS_POSIX_TIME_H */
