#ifndef BEZOS_POSIX_ASSERT_H
#define BEZOS_POSIX_ASSERT_H 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#   define assert(x) ((void)0)
#else
    /* TODO: implement assert */
#   define assert(x) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_ASSERT_H */
