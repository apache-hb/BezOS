#include "clock.hpp"

#include "timer/tick_source.hpp"

static constexpr auto kGregorianReform = std::chrono::years{1582} + std::chrono::months{10} + std::chrono::days{15};

static km::timestep DateToInstant(km::DateTime date) {
    auto timepoint = std::chrono::years{date.year}
              + std::chrono::months{date.month}
              + std::chrono::days{date.day}
              + std::chrono::hours{date.hour}
              + std::chrono::minutes{date.minute}
              + std::chrono::seconds{date.second};

    return std::chrono::duration_cast<km::timestep>(timepoint - kGregorianReform);
}

km::Clock::Clock(DateTime start, ITickSource *counter)
    : mCounter(counter)
    , mStartDate(DateToInstant(start))
    , mStartTicks(counter->ticks())
    , mFrequency(counter->frequency())
{ }

OsStatus km::Clock::stat(OsClockInfo *result) {
    auto name = [&] -> stdx::StringView {
        switch (mCounter->type()) {
        case km::TickSourceType::PIT8254:
            return "Programmable Interval Timer";
        case km::TickSourceType::HPET:
            return "High Precision Multimedia Timer";
        case km::TickSourceType::APIC:
            return "Local APIC Timer";
        case km::TickSourceType::TSC:
            return "Invariant TSC";
        default:
            return "Unknown";
        }
    }();

    OsClockInfo info {
        .FrequencyHz = uint64_t(mFrequency / si::hertz),
        .BootTime = mStartDate.count(),
    };

    std::copy(name.begin(), name.end(), info.DisplayName);
    *result = info;
    return OsStatusSuccess;
}

OsStatus km::Clock::time(OsInstant *result) {
    auto now = mCounter->ticks();
    auto ticks = now - mStartTicks;
    auto nanos = (ticks * 1'000'000'000) / mFrequency;

    *result = OsInstant(mStartDate.count()) + OsInstant((nanos / 100) * si::hertz);
    return OsStatusSuccess;
}

OsStatus km::Clock::ticks(OsTickCounter *result) {
    *result = mCounter->ticks();
    return OsStatusSuccess;
}
