#include <bezos/facility/threads.h>

#include <bezos/private.h>

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

OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexCreate, (uintptr_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsMutexHandle)result.Value;
    return result.Status;
}

OsStatus OsMutexDestroy(OsMutexHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexDestroy, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsMutexLock(OsMutexHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexLock, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsMutexUnlock(OsMutexHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexUnlock, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}
