#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_CLOCK_NAME_MAX 64

typedef uint64_t OsClockFrequency;

struct OsClockInfo {
    /// @brief Display name of the clock.
    OsUtf8Char DisplayName[OS_CLOCK_NAME_MAX];

    /// @brief The frequency of the clock.
    /// @note This may not be accurate, it is provided as a best effort.
    OsClockFrequency FrequencyHz;

    OsInstant BootTime;
};

extern OsStatus OsClockGetTime(OsInstant *OutTime);

extern OsStatus OsClockStat(struct OsClockInfo *OutInfo);

extern OsStatus OsClockGetTicks(OsTickCounter *OutTicks);

#ifdef __cplusplus
}
#endif
