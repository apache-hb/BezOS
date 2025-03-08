#include <posix/math.h>

#include <posix/errno.h>

double fmod(double, double) noexcept {
    errno = ENOSYS;
    return 0.0;
}

float fmodf(float, float) noexcept {
    errno = ENOSYS;
    return 0.0f;
}

long double fmodl(long double, long double) noexcept {
    errno = ENOSYS;
    return 0.0l;
}

double pow(double, double) noexcept {
    errno = ENOSYS;
    return 0.0;
}

float powf(float, float) noexcept {
    errno = ENOSYS;
    return 0.0f;
}

long double powl(long double, long double) noexcept {
    errno = ENOSYS;
    return 0.0l;
}
