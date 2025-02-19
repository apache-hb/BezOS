#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsClockStat {
    uint64_t FrequencyHz;
};

extern OsStatus OsClockGetTime(OsInstant *OutTime);

extern OsStatus OsClockStat(struct OsClockStat *OutStat);

#ifdef __cplusplus
}
#endif
