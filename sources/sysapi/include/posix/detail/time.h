#ifndef BEZOS_POSIX_DETAIL_TIME_H
#define BEZOS_POSIX_DETAIL_TIME_H 1

typedef long time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#endif /* BEZOS_POSIX_DETAIL_TIME_H */
