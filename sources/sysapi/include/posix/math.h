#ifndef BEZOS_POSIX_MATH_H
#define BEZOS_POSIX_MATH_H 1

#include <detail/cxx.h>
#include <detail/math.h>

BZP_API_BEGIN

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))
#define HUGE_VAL (__builtin_huge_valf())

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

extern double fmod(double, double) BZP_NOEXCEPT;
extern float fmodf(float, float) BZP_NOEXCEPT;
extern long double fmodl(long double, long double) BZP_NOEXCEPT;

extern double pow(double, double) BZP_NOEXCEPT;
extern float powf(float, float) BZP_NOEXCEPT;
extern long double powl(long double, long double) BZP_NOEXCEPT;

extern int isinf(double) BZP_NOEXCEPT;
#define isinf isinf

BZP_API_END

#endif /* BEZOS_POSIX_MATH_H */
