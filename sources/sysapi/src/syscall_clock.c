#include <bezos/facility/clock.h>

#include <bezos/private.h>

OsStatus OsClockGetTime(OsInstant *OutTime) {
    struct OsCallResult result = OsSystemCall(eOsCallClockGetTime, 0, 0, 0, 0);
    *OutTime = (OsInstant)result.Value;
    return result.Status;
}

OsStatus OsClockStat(struct OsClockStat *OutStat) {
    struct OsCallResult result = OsSystemCall(eOsCallClockStat, (uintptr_t)OutStat, 0, 0, 0);
    return result.Status;
}
