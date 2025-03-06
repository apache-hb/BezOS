#ifndef BEZOS_POSIX_DETAIL_ATTRIBUTES_H
#define BEZOS_POSIX_DETAIL_ATTRIBUTES_H 1

#if defined(__has_attribute)
#   define BZP_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#   define BZP_HAS_ATTRIBUTE(x) 0
#endif

#if defined(__cplusplus)
#   define BZP_NORETURN [[noreturn]]
#else
#   define BZP_NORETURN _Noreturn
#endif

#if BZP_HAS_ATTRIBUTE(nonnull)
#   define BZP_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#   define BZP_NONNULL_ALL __attribute__((nonnull))
#else
#   define BZP_NONNULL(...)
#   define BZP_NONNULL_ALL
#endif

#if BZP_HAS_ATTRIBUTE(returns_nonnull)
#   define BZP_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#   define BZP_RETURNS_NONNULL
#endif

#if defined(__GNUC__) || defined(__clang__)
#   define BZP_RESTRICT __restrict__
#else
#   define BZP_RESTRICT
#endif

#endif /* BEZOS_POSIX_DETAIL_ATTRIBUTES_H */
