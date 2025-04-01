#include <bezos/facility/mutex.h>

#include <bezos/private.h>

OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsMutexHandle)result.Value;
    return result.Status;
}

OsStatus OsMutexDestroy(OsMutexHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexDestroy, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsMutexLock(OsMutexHandle Handle, OsInstant Timeout) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexLock, (uint64_t)Handle, Timeout, 0, 0);
    return result.Status;
}

OsStatus OsMutexUnlock(OsMutexHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexUnlock, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsMutexStat(OsMutexHandle Handle, struct OsMutexInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallMutexStat, (uint64_t)Handle, (uint64_t)OutInfo, 0, 0);
    return result.Status;
}
