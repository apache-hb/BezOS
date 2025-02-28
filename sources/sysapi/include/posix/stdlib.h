#ifndef BEZOS_POSIX_STDLIB_H
#define BEZOS_POSIX_STDLIB_H 1

#include <detail/size.h>
#include <detail/null.h>

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn]]
extern void abort(void);

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

extern void *malloc(size_t);
extern void *calloc(size_t, size_t);
extern void *realloc(void *, size_t);
extern void free(void *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STDLIB_H */
