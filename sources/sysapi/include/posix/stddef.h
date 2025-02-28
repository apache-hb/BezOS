#ifndef BEZOS_POSIX_STDDEF_H
#define BEZOS_POSIX_STDDEF_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

#ifndef NULL
#   define NULL ((void*)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STDDEF_H */
