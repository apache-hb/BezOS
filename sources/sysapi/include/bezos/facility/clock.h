#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsClockFrequency;

struct OsClockStat {
    /// @brief Display name of the clock.
    OsUtf8Char DisplayName[OS_DEVICE_NAME_MAX];

    /// @brief The frequency of the clock.
    /// @note This may not be accurate, it is provided as a best effort.
    OsClockFrequency FrequencyHz;
};

extern OsStatus OsClockGetTime(OsInstant *OutTime);

extern OsStatus OsClockStat(struct OsClockStat *OutStat);

#ifdef __cplusplus
}
#endif
