#include <bezos/facility/process.h>

#include <bezos/private.h>

OsStatus OsProcessCurrent(OsProcessHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallProcessCurrent, 0, 0, 0, 0);
    *OutHandle = (OsProcessHandle)result.Value;
    return result.Status;
}

OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallProcessCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsProcessHandle)result.Value;
    return result.Status;
}

OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend) {
    struct OsCallResult result = OsSystemCall(eOsCallProcessSuspend, (uint64_t)Handle, Suspend, 0, 0);
    return result.Status;
}

OsStatus OsProcessTerminate(OsProcessHandle Handle, int64_t ExitCode) {
    struct OsCallResult result = OsSystemCall(eOsCallProcessDestroy, (uint64_t)Handle, ExitCode, 0, 0);
    return result.Status;
}

OsStatus OsProcessStat(OsProcessHandle Handle, struct OsProcessInfo *OutState) {
    struct OsCallResult result = OsSystemCall(eOsCallProcessStat, (uint64_t)Handle, (uint64_t)OutState, 0, 0);
    return result.Status;
}
