#include <bezos/facility/handle.h>

#include <bezos/private.h>

OsStatus OsHandleWait(OsHandle Handle, OsInstant Timeout) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleWait, (uint64_t)Handle, Timeout, 0, 0);
    return result.Status;
}

OsStatus OsHandleClone(OsHandle Handle, struct OsHandleCloneInfo CloneInfo, OsHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleClone, (uint64_t)Handle, (uint64_t)&CloneInfo, 0, 0);
    *OutHandle = (OsHandle)result.Value;
    return result.Status;
}

OsStatus OsHandleClose(OsHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleClose, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsHandleStat(OsHandle Handle, struct OsHandleInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleStat, (uint64_t)Handle, (uint64_t)OutInfo, 0, 0);
    return result.Status;
}

OsStatus OsHandleOpen(OsObject Object, OsHandleAccess Access, OsHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleOpen, (uint64_t)Object, Access, 0, 0);
    *OutHandle = (OsHandle)result.Value;
    return result.Status;
}
