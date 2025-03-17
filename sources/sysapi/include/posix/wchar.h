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

extern int swprintf(wchar_t * BZP_RESTRICT, size_t, const wchar_t * BZP_RESTRICT, ...) BZP_NOEXCEPT;

extern long wcstol(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT, int) BZP_NOEXCEPT;
extern long long wcstoll(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT, int) BZP_NOEXCEPT;
extern unsigned long wcstoul(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT, int) BZP_NOEXCEPT;
extern unsigned long long wcstoull(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT, int) BZP_NOEXCEPT;

extern double wcstod(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT) BZP_NOEXCEPT;
extern long double wcstold(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT) BZP_NOEXCEPT;
extern float wcstof(const wchar_t * BZP_RESTRICT, wchar_t ** BZP_RESTRICT) BZP_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_WCHAR_H */
