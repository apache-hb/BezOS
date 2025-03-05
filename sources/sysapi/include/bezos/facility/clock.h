#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsClockFrequency;

struct OsClockStat {
    OsClockFrequency FrequencyHz;
};

extern OsStatus OsClockGetTime(OsInstant *OutTime);

extern OsStatus OsClockStat(struct OsClockStat *OutStat);

#ifdef __cplusplus
}
#endif
