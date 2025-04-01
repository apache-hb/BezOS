#include <bezos/facility/clock.h>

#include <bezos/private.h>

OsStatus OsClockGetTime(OsInstant *OutTime) {
    struct OsCallResult result = OsSystemCall(eOsCallClockGetTime, 0, 0, 0, 0);
    *OutTime = (OsInstant)result.Value;
    return result.Status;
}

OsStatus OsClockStat(struct OsClockInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallClockStat, (uint64_t)OutInfo, 0, 0, 0);
    return result.Status;
}
