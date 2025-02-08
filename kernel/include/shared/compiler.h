#pragma once

#ifdef __cplusplus
#   define OS_STATIC_ASSERT(expr, message) static_assert(expr, message)
#else
#   define OS_STATIC_ASSERT(expr, message) _Static_assert(expr, message)
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
