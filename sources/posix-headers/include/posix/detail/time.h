#ifndef BEZOS_POSIX_DETAIL_TIME_H
#define BEZOS_POSIX_DETAIL_TIME_H 1

typedef long time_t;
typedef long suseconds_t;

typedef long clock_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

#endif /* BEZOS_POSIX_DETAIL_TIME_H */
