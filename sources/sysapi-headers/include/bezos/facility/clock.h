#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsClock Clock
/// @{

/// @brief The max size of a clock name.
#define OS_CLOCK_NAME_MAX 64

typedef uint64_t OsClockFrequencyHz;

/// @brief Information about a clock.
struct OsClockInfo {
    /// @brief Display name of the clock.
    OsUtf8Char DisplayName[OS_CLOCK_NAME_MAX];

    /// @brief The frequency of the clock.
    /// @note This may not be accurate, it is provided as a best effort.
    OsClockFrequencyHz FrequencyHz;

    /// @brief The time this clock was started.
    OsInstant BootTime;
};

/// @brief Get the current time of the system clock.
///
/// @param OutTime The current time of the system clock.
///
/// @return The status of the operation.
extern OsStatus OsClockGetTime(OsInstant *OutTime);

/// @brief Get the current number of ticks of the system clock.
///
/// @note Garanteed to be monotonically increasing.
///
/// @param[out] OutTicks The current number of ticks of the system clock.
///
/// @return The status of the operation.
extern OsStatus OsClockGetTicks(OsTickCounter *OutTicks);

/// @brief Get information about the system time source.
///
/// @param[out] OutInfo The information about the system clock.
///
/// @return The status of the operation.
extern OsStatus OsClockStat(struct OsClockInfo *OutInfo);

/// @} // group OsClock

#ifdef __cplusplus
}
#endif
