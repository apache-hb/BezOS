#pragma once

#include <bezos/facility/clock.h>

#include "cmos.hpp"
#include "units.hpp"

#include <chrono>

namespace km {
    class ITickSource;

    using os_instant = std::chrono::duration<OsInstant, std::ratio<1LL, 10000000LL>>;

    class Clock {
        ITickSource *mCounter;
        os_instant mStartDate;
        OsTickCounter mStartTicks;
        mp::quantity<si::hertz, uint64_t> mFrequency;

    public:
        Clock(DateTime start, ITickSource *counter);

        constexpr Clock() = default;

        OsStatus stat(OsClockInfo *result);
        OsStatus time(OsInstant *result);
        OsStatus ticks(OsTickCounter *result);
    };
}
