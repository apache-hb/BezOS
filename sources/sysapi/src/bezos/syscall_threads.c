#include <bezos/facility/threads.h>

#include <bezos/private.h>

OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadCreate, (uintptr_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsThreadHandle)result.Value;
    return result.Status;
}

OsStatus OsThreadDestroy(OsThreadHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadDestroy, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsThreadCurrent(OsThreadHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadCurrent, 0, 0, 0, 0);
    *OutHandle = (OsThreadHandle)result.Value;
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
    struct OsCallResult result = OsSystemCall(eOsCallThreadSuspend, (uintptr_t)Handle, Suspend, 0, 0);
    return result.Status;
}

OsStatus OsThreadStat(OsThreadHandle Handle, struct OsThreadInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallThreadStat, (uintptr_t)Handle, (uintptr_t)OutInfo, 0, 0);
    return result.Status;
}
