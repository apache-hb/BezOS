#ifndef BEZOS_POSIX_STDINT_H
#define BEZOS_POSIX_STDINT_H 1

#include <limits.h> /* IWYU pragma: export */

#ifdef __cplusplus
extern "C" {
#endif

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;

typedef __INT_LEAST8_TYPE__ int_least8_t;
typedef __INT_LEAST16_TYPE__ int_least16_t;
typedef __INT_LEAST32_TYPE__ int_least32_t;
typedef __INT_LEAST64_TYPE__ int_least64_t;

typedef __INT_FAST8_TYPE__ int_fast8_t;
typedef __INT_FAST16_TYPE__ int_fast16_t;
typedef __INT_FAST32_TYPE__ int_fast32_t;
typedef __INT_FAST64_TYPE__ int_fast64_t;

typedef __INTMAX_TYPE__ intmax_t;
typedef __INTPTR_TYPE__ intptr_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __UINT_LEAST8_TYPE__ uint_least8_t;
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
typedef __UINT_LEAST64_TYPE__ uint_least64_t;

typedef __UINT_FAST8_TYPE__ uint_fast8_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;

typedef __UINTMAX_TYPE__ uintmax_t;
typedef __UINTPTR_TYPE__ uintptr_t;

#define INT8_C(x) ((int8_t)(x))
#define INT16_C(x) ((int16_t)(x))
#define INT32_C(x) ((int32_t)(x##ll))
#define INT64_C(x) ((int64_t)(x##ll))

#define INTMAX_C(x) ((intmax_t)(x##ll))

#define UINT8_C(x) ((uint8_t)(x))
#define UINT16_C(x) ((uint16_t)(x))
#define UINT32_C(x) ((uint32_t)(x##ull))
#define UINT64_C(x) ((uint64_t)(x##ull))

#define UINTMAX_C(x) ((uintmax_t)(x##ull))

#define SIZE_MAX __SIZE_MAX__

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STDINT_H */
