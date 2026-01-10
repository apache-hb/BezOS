#ifndef BEZOS_POSIX_DETAIL_TYPES_WINT_T_H
#define BEZOS_POSIX_DETAIL_TYPES_WINT_T_H 1

#ifdef __WINT_TYPE__
#   define __jlibc_wint_type __WINT_TYPE__
#else
#   define __jlibc_wint_type unsigned int
#endif

typedef __jlibc_wint_type wint_t;

#endif /* BEZOS_POSIX_DETAIL_TYPES_WINT_T_H */
