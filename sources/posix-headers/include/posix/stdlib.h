#ifndef BEZOS_POSIX_STDLIB_H
#define BEZOS_POSIX_STDLIB_H 1

#include <detail/cxx.h>
#include <detail/attributes.h>
#include <detail/size.h>
#include <detail/null.h>
#include <detail/math.h>

BZP_API_BEGIN

#define MB_CUR_MAX 1

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX 32767

__attribute__((__nothrow__, __noreturn__))
extern void abort(void);

__attribute__((__nothrow__, __noreturn__))
extern void exit(int);

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

extern div_t div(int, int);
extern ldiv_t ldiv(long int, long int);
extern lldiv_t lldiv(long long int, long long int);

extern double atof(const char *) __BZ_NONNULL(1);

extern int atoi(const char *) __BZ_NONNULL(1);
extern long int atol(const char *) __BZ_NONNULL(1);
extern long long int atoll(const char *) __BZ_NONNULL(1);

extern double strtod(const char *, char **);
extern float strtof(const char *, char **);
extern long double strtold(const char *, char **);

extern long strtol(const char *, char **, int);
extern long long strtoll(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);
extern unsigned long long strtoull(const char *, char **, int);

__attribute__((__nothrow__, __malloc__, __alloc_size__(1), __allocating__))
extern void *malloc(size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(2), __alloc_align__(1), __allocating__))
extern void *aligned_alloc(size_t, size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(1, 2), __allocating__))
extern void *calloc(size_t, size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(2), __allocating__))
extern void *realloc(void *, size_t);

__attribute__((__nothrow__, __nonnull__, __nonallocating__))
extern void free(void *);

extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *)); /* Not noexcept, callback may throw when used in c++ */

extern char *getenv(const char *) __BZ_NONNULL(1);

extern int putenv(char *) __BZ_NONNULL(1);

extern int setenv(const char *, const char *, int) __BZ_NONNULL(1, 2);
extern int unsetenv(const char *) __BZ_NONNULL(1);

extern void srand(unsigned);
extern int rand(void);
extern int rand_r(unsigned *) __BZ_NONNULL(1);

extern char *mkdtemp(char *);
extern int mkstemp(char *);
extern char *mktemp(char *);

extern int atexit(void (*)(void));

BZP_API_END

#endif /* BEZOS_POSIX_STDLIB_H */
