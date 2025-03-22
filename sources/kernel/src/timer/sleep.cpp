#include "timer/tick_source.hpp"

void km::BusySleep(ITickSource *timer, std::chrono::microseconds duration) {
    auto frequency = timer->frequency();
    uint64_t ticks = (uint64_t(frequency / si::hertz) * duration.count()) / 1'000'000;
    uint64_t now = timer->ticks();
    uint64_t then = now + ticks;
    while (timer->ticks() < then) {
        _mm_pause();
    }
}
