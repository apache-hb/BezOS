#include "clock.hpp"

#include "logger/categories.hpp"
#include "timer/tick_source.hpp"

static constexpr auto kGregorianReform = std::chrono::years{1582} + std::chrono::months{10} + std::chrono::days{15};

// calculate number of 100ns intervals since the gregorian reform
static constexpr km::os_instant DateToInstant(km::DateTime date) {
    auto timepoint = std::chrono::years{date.year}
              + std::chrono::months{date.month}
              + std::chrono::days{date.day}
              + std::chrono::hours{date.hour}
              + std::chrono::minutes{date.minute}
              + std::chrono::seconds{date.second};

    return std::chrono::duration_cast<km::os_instant>(timepoint - kGregorianReform);
}

#if 0
static constexpr km::DateTime InstantToDate(OsInstant instant) {
    auto timepoint = km::os_instant(instant);
    auto time = timepoint + kGregorianReform;

    auto years = std::chrono::floor<std::chrono::years>(time);
    auto months = std::chrono::floor<std::chrono::months>(time - years);
    auto days = std::chrono::floor<std::chrono::days>(time - years - months);
    auto hours = std::chrono::floor<std::chrono::hours>(time - years - months - days);
    auto minutes = std::chrono::floor<std::chrono::minutes>(time - years - months - days - hours);
    auto seconds = std::chrono::floor<std::chrono::seconds>(time - years - months - days - hours - minutes);

    return km::DateTime {
        .second = uint8_t(seconds.count()),
        .minute = uint8_t(minutes.count()),
        .hour = uint8_t(hours.count()),
        .day = uint8_t(days.count()),
        .month = uint8_t(months.count()),
        .year = uint16_t(years.count()),
    };
}
#endif

static constexpr OsInstant TimeSinceBoot(km::os_instant bootTime, km::hertz frequency, OsTickCounter bootTicks, OsTickCounter currentTicks) {
    auto diff = currentTicks - bootTicks;
    auto nanos = (diff * 1'000'000'0) / frequency;
    return OsInstant(bootTime.count()) + OsInstant(nanos * si::hertz);
}

static_assert(DateToInstant(km::DateTime{.day = 15, .month = 10, .year = 1582}) == km::os_instant(0));
static_assert(DateToInstant(km::DateTime{.day = 16, .month = 10, .year = 1582}) == km::os_instant(1ll * 24 * 60 * 60 * 10000000));

km::Clock::Clock(DateTime start, ITickSource *counter)
    : mCounter(counter)
    , mStartDate(DateToInstant(start))
    , mStartTicks(counter->ticks())
    , mFrequency(counter->frequency())
{
    ClockLog.infof("Boot time: ", start.year, "-", start.month, "-", start.day, "T", start.hour, ":", start.minute, ":", start.second, "Z");
    ClockLog.infof("Boot ticks: ", mStartTicks);
    ClockLog.infof("Frequency: ", mFrequency);
}

OsStatus km::Clock::stat(OsClockInfo *result) {
    auto name = [&] -> stdx::StringView {
        switch (mCounter->type()) {
        case km::TickSourceType::PIT8254:
            return "Programmable Interval Timer 8254";
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
    auto ticks = mCounter->ticks();
    OsInstant now = TimeSinceBoot(mStartDate, mFrequency, mStartTicks, ticks);
    *result = now;
    return OsStatusSuccess;
}

OsStatus km::Clock::ticks(OsTickCounter *result) {
    *result = mCounter->ticks();
    return OsStatusSuccess;
}
