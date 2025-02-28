#ifndef BEZOS_POSIX_WCHAR_H
#define BEZOS_POSIX_WCHAR_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mbstate_t;

typedef __WINT_TYPE__ wint_t;

#ifndef WEOF
#   define WEOF (0xffffffffu)
#endif

extern wchar_t *wcschr(const wchar_t *, wchar_t);

extern wchar_t *wcspbrk(const wchar_t *, const wchar_t *);

extern wchar_t *wcsrchr(const wchar_t *, wchar_t);

extern wchar_t *wcsstr(const wchar_t *, const wchar_t *);

extern wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_WCHAR_H */
