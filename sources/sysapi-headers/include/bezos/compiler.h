#pragma once

#if defined(__cplusplus)
#   define OS_STATIC_ASSERT(expr, message) static_assert(expr, message)
#   define OS_EXTERN extern "C"
#   define OS_BEGIN_API extern "C" {
#   define OS_END_API }
#   define OS_NORETURN [[noreturn]]
#else
#   define OS_STATIC_ASSERT(expr, message) _Static_assert(expr, message)
#   define OS_EXTERN
#   define OS_BEGIN_API
#   define OS_END_API
#   define OS_NORETURN _Noreturn
#endif

#ifdef __has_attribute
#   define OS_HAS_ATTRIBUTE(attr) __has_attribute(attr)
#else
#   define OS_HAS_ATTRIBUTE(attr) 0
#endif

#if OS_HAS_ATTRIBUTE(counted_by) && !defined(__cplusplus)
#   define OS_COUNTED_BY(expr) __attribute__((counted_by(expr)))
#else
#   define OS_COUNTED_BY(expr)
#endif
