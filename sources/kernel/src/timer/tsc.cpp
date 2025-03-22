#include "timer/tsc_timer.hpp"

using namespace std::chrono_literals;

static constexpr std::chrono::milliseconds kTrainDuration = 10ms;
static constexpr auto kTrainSteps = 10;

OsStatus km::TrainInvariantTsc(ITickSource *refclk, InvariantTsc *timer) {
    uint64_t sum = 0;

    for (auto i = 0; i < kTrainSteps; i++) {
        uint64_t now = __rdtsc();
        BusySleep(refclk, kTrainDuration);
        uint64_t then = __rdtsc();

        sum += (then - now);
    }

    auto totalTime = (kTrainSteps * kTrainDuration);

    // ticks per ms
    auto msFreq = (sum / totalTime.count()) * si::hertz;

    // ticks per second
    auto freq = msFreq * 1'000;

    *timer = InvariantTsc { freq };
    return OsStatusSuccess;
}
