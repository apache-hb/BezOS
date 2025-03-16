#ifndef BEZOS_POSIX_STDDEF_H
#define BEZOS_POSIX_STDDEF_H 1

#include <detail/size.h>
#include <detail/null.h>

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

typedef long double max_align_t;

#ifndef offsetof
#   define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#endif /* BEZOS_POSIX_STDDEF_H */
