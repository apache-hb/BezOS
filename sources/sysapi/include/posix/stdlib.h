#ifndef BEZOS_POSIX_STDLIB_H
#define BEZOS_POSIX_STDLIB_H 1

#include <detail/cxx.h>
#include <detail/attributes.h>
#include <detail/size.h>
#include <detail/null.h>

BZP_API_BEGIN

#define MB_CUR_MAX 1

BZP_NORETURN
extern void abort(void) BZP_NOEXCEPT;

BZP_NORETURN
extern void exit(int) BZP_NOEXCEPT;

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long int quot;
    long int rem;
} ldiv_t;

typedef struct {
    long long int quot;
    long long int rem;
} lldiv_t;

extern div_t div(int, int) BZP_NOEXCEPT;
extern ldiv_t ldiv(long int, long int) BZP_NOEXCEPT;
extern lldiv_t lldiv(long long int, long long int) BZP_NOEXCEPT;

extern void *malloc(size_t) BZP_NOEXCEPT;
extern void *calloc(size_t, size_t) BZP_NOEXCEPT;
extern void *realloc(void *, size_t) BZP_NOEXCEPT;
extern void free(void *) BZP_NOEXCEPT BZP_NONNULL(1);

extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *)); /* Not noexcept, callback may throw when used in c++ */

extern char *getenv(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern int atoi(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern long strtol(const char *, char **, int) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_STDLIB_H */
