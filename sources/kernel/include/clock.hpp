#pragma once

#include <bezos/facility/clock.h>

#include "cmos.hpp"
#include "units.hpp"

#include <chrono>

namespace km {
    class ITickSource;

    using timestep = std::chrono::duration<int64_t, std::ratio<1LL, 10000000LL>>;

    class Clock {
        ITickSource *mCounter;
        timestep mStartDate;
        OsInstant mStartTicks;
        mp::quantity<si::hertz, uint64_t> mFrequency;

    public:
        Clock(DateTime start, ITickSource *counter);

        constexpr Clock() = default;

        OsStatus stat(OsClockInfo *result);
        OsStatus time(OsInstant *result);
        OsStatus ticks(OsTickCounter *result);
    };
}
