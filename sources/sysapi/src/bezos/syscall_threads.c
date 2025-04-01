#include <bezos/facility/threads.h>

#include <bezos/private.h>

OsStatus OsThreadCurrent(OsHandleAccess Access, OsThreadHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadCurrent, (uint64_t)Access, 0, 0, 0);
    *OutHandle = (OsThreadHandle)result.Value;
    return result.Status;
}

OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsThreadHandle)result.Value;
    return result.Status;
}

OsStatus OsThreadDestroy(OsThreadHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadDestroy, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsThreadYield(void) {
    return OsThreadSleep(0);
}

OsStatus OsThreadSleep(OsDuration Duration) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadSleep, Duration, 0, 0, 0);
    return result.Status;
}

OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadSuspend, (uint64_t)Handle, Suspend, 0, 0);
    return result.Status;
}

OsStatus OsThreadStat(OsThreadHandle Handle, struct OsThreadInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadStat, (uint64_t)Handle, (uint64_t)OutInfo, 0, 0);
    return result.Status;
}
