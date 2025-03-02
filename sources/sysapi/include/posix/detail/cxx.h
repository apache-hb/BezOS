#ifndef BEZOS_POSIX_DETAIL_CXX_H
#define BEZOS_POSIX_DETAIL_CXX_H 1

#ifdef __cplusplus
#   define BEZOS_POSIX_NOEXCEPT noexcept
#   define BEZOS_POSIX_API_BEGIN extern "C" {
#   define BEZOS_POSIX_API_END }
#else
#   define BEZOS_POSIX_NOEXCEPT
#   define BEZOS_POSIX_API_BEGIN
#   define BEZOS_POSIX_API_END
#endif

#endif /* BEZOS_POSIX_DETAIL_CXX_H */
