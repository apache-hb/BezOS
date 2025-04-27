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

extern double atof(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern int atoi(const char *) BZP_NOEXCEPT BZP_NONNULL(1);
extern long int atol(const char *) BZP_NOEXCEPT BZP_NONNULL(1);
extern long long int atoll(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern double strtod(const char *, char **) BZP_NOEXCEPT;
extern float strtof(const char *, char **) BZP_NOEXCEPT;
extern long double strtold(const char *, char **) BZP_NOEXCEPT;

extern long strtol(const char *, char **, int) BZP_NOEXCEPT;
extern long long strtoll(const char *, char **, int) BZP_NOEXCEPT;
extern unsigned long strtoul(const char *, char **, int) BZP_NOEXCEPT;
extern unsigned long long strtoull(const char *, char **, int) BZP_NOEXCEPT;

__attribute__((__nothrow__, __malloc__, __alloc_size__(1)))
extern void *malloc(size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(2)))
extern void *aligned_alloc(size_t, size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(1, 2)))
extern void *calloc(size_t, size_t);

__attribute__((__nothrow__, __malloc__, __alloc_size__(2)))
extern void *realloc(void *, size_t);

__attribute__((__nothrow__))
extern void free(void *) BZP_NONNULL(1);

extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *)); /* Not noexcept, callback may throw when used in c++ */

extern char *getenv(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern int putenv(char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern int setenv(const char *, const char *, int) BZP_NOEXCEPT BZP_NONNULL(1, 2);
extern int unsetenv(const char *) BZP_NOEXCEPT BZP_NONNULL(1);

extern void srand(unsigned) BZP_NOEXCEPT;
extern int rand(void) BZP_NOEXCEPT;
extern int rand_r(unsigned *) BZP_NOEXCEPT BZP_NONNULL(1);

extern char *mkdtemp(char *) BZP_NOEXCEPT;
extern int mkstemp(char *) BZP_NOEXCEPT;
extern char *mktemp(char *) BZP_NOEXCEPT;

extern int atexit(void (*)(void)) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_STDLIB_H */
