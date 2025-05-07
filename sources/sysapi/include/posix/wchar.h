#ifndef BEZOS_POSIX_WCHAR_H
#define BEZOS_POSIX_WCHAR_H 1

#include <stddef.h>

#include <detail/file.h>
#include <detail/attributes.h>
#include <detail/cxx.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tm;

typedef struct mbstate_t { void *state; } mbstate_t;

typedef __WINT_TYPE__ wint_t;

#ifndef WEOF
#   define WEOF (0xffffffffu)
#endif

extern wchar_t *wcschr(const wchar_t *, wchar_t);

extern wchar_t *wcspbrk(const wchar_t *, const wchar_t *);

extern wchar_t *wcsrchr(const wchar_t *, wchar_t);

extern wchar_t *wcsstr(const wchar_t *, const wchar_t *);

extern wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);

extern int wmemcmp(const wchar_t *, const wchar_t *, size_t);

extern size_t wcslen(const wchar_t *);

extern int iswspace(wint_t);

extern int swprintf(wchar_t *__BZ_RESTRICT, size_t, const wchar_t *__BZ_RESTRICT, ...);

extern long wcstol(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT, int);
extern long long wcstoll(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT, int);
extern unsigned long wcstoul(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT, int);
extern unsigned long long wcstoull(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT, int);

extern double wcstod(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT);
extern long double wcstold(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT);
extern float wcstof(const wchar_t *__BZ_RESTRICT, wchar_t **__BZ_RESTRICT);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_WCHAR_H */
