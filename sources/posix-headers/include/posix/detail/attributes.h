#ifndef BEZOS_POSIX_DETAIL_ATTRIBUTES_H
#define BEZOS_POSIX_DETAIL_ATTRIBUTES_H 1

#if defined(__has_attribute)
#   define __BZ_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#   define __BZ_HAS_ATTRIBUTE(x) 0
#endif

#if defined(__has_feature)
#   define __BZ_HAS_FEATURE(x) __has_feature(x)
#else
#   define __BZ_HAS_FEATURE(x) 0
#endif

#if defined(__cplusplus)
#   define __BZ_NORETURN [[noreturn]]
#else
#   define __BZ_NORETURN _Noreturn
#endif

#if __BZ_HAS_ATTRIBUTE(__nonnull__)
#   define __BZ_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#   define __BZ_NONNULL_ALL __attribute__((__nonnull__))
#else
#   define __BZ_NONNULL(...)
#   define __BZ_NONNULL_ALL
#endif

#if __BZ_HAS_ATTRIBUTE(__returns_nonnull__)
#   define __BZ_RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
#   define __BZ_RETURNS_NONNULL
#endif

#if __BZ_HAS_ATTRIBUTE(__allocating__) && __BZ_HAS_ATTRIBUTE(__blocking__)
#   define __BZ_ALLOCATING __attribute__((__allocating__))
#   define __BZ_NONALLOCATING __attribute__((__nonallocating__))
#   define __BZ_BLOCKING __attribute__((__blocking__))
#   define __BZ_NONBLOCKING __attribute__((__nonblocking__))
#else
#   define __BZ_ALLOCATING
#   define __BZ_NONALLOCATING
#   define __BZ_BLOCKING
#   define __BZ_NONBLOCKING
#endif

#if defined(__GNUC__) || defined(__clang__)
#   define __BZ_RESTRICT __restrict__
#else
#   define __BZ_RESTRICT
#endif

#if __BZ_HAS_FEATURE(bounds_safety)
#   define __BZ_COUNTED_BY(x) __attribute__((__counted_by__(x)))
#   define __BZ_SIZED_BY(x) __attribute__((__sized_by__(x)))
#   define __BZ_ZSTRING __attribute__((__null_terminated))
#else
#   define __BZ_COUNTED_BY(x)
#   define __BZ_SIZED_BY(x)
#   define __BZ_ZSTRING
#endif

#endif /* BEZOS_POSIX_DETAIL_ATTRIBUTES_H */
