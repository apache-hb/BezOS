#include "timer/apic_timer.hpp"

#include "apic.hpp"

using namespace std::chrono_literals;

static constexpr std::chrono::milliseconds kTrainDuration = 10ms;
static constexpr auto kTrainSteps = 10;

uint64_t km::ApicTimer::ticks() const {
    return mApic->getCurrentCount();
}

OsStatus km::trainApicTimer(IApic *apic, ITickSource *refclk, ApicTimer *timer) {
    apic->setTimerDivisor(apic::TimerDivide::e1);

    uint64_t sum = 0;

    for (auto i = 0; i < kTrainSteps; i++) {
        apic->setInitialCount(UINT32_MAX);
        BusySleep(refclk, kTrainDuration);
        uint64_t then = apic->getCurrentCount();

        sum += (UINT32_MAX - then);
    }

    apic->setInitialCount(0);

    auto totalTime = (kTrainSteps * kTrainDuration);

    // ticks per ms
    auto msFreq = (sum / totalTime.count()) * si::hertz;

    // ticks per second
    auto freq = msFreq * 1'000;

    *timer = ApicTimer { freq, apic };
    return OsStatusSuccess;
}
