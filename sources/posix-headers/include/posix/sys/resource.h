#ifndef BEZOS_POSIX_SYS_RESOURCE_H
#define BEZOS_POSIX_SYS_RESOURCE_H 1

#include <detail/cxx.h>
#include <detail/size.h>
#include <detail/node.h>
#include <detail/id.h>
#include <detail/time.h>

#include <features.h>

BZP_API_BEGIN

typedef unsigned long int rlim_t;

#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

#define RLIM_INFINITY ((rlim_t)(ULONG_MAX - 0))
#define RLIM_SAVED_MAX ((rlim_t)(ULONG_MAX - 1))
#define RLIM_SAVED_CUR ((rlim_t)(ULONG_MAX - 2))

#define RLIMIT_CPU 0
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_RSS 5
#define RLIMIT_MEMLOCK 6
#define RLIMIT_NPROC 7
#define RLIMIT_OFILE 8
#define RLIMIT_NOFILE 9
#define RLIMIT_AS 10

#define RLIM_NLIMITS 16

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN 1

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;

#if JEFF_OPTION_LINUX
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
#endif
};

extern int getpriority(int, id_t);
extern int getrlimit(int, struct rlimit*);
extern int getrusage(int, struct rusage*);
extern int setpriority(int, id_t, int);
extern int setrlimit(int, const struct rlimit*);

BZP_API_END

#endif /* BEZOS_POSIX_SYS_RESOURCE_H */
