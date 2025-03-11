#include <posix/math.h>

#include "private.hpp"

double fmod(double, double) noexcept {
    Unimplemented();
    return 0.0;
}

float fmodf(float, float) noexcept {
    Unimplemented();
    return 0.0f;
}

long double fmodl(long double, long double) noexcept {
    Unimplemented();
    return 0.0l;
}

double pow(double, double) noexcept {
    Unimplemented();
    return 0.0;
}

float powf(float, float) noexcept {
    Unimplemented();
    return 0.0f;
}

long double powl(long double, long double) noexcept {
    Unimplemented();
    return 0.0l;
}
