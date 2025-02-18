#include <bezos/facility/vmem.h>

#include <bezos/private.h>

OsStatus OsAddressSpaceCreate(struct OsAddressSpaceCreateInfo CreateInfo, OsAddressSpaceHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallAddressSpaceCreate, (uintptr_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsAddressSpaceHandle)result.Value;
    return result.Status;
}

OsStatus OsAddressSpaceDestroy(OsAddressSpaceHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallAddressSpaceDestroy, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsAddressSpaceStat(OsAddressSpaceHandle Handle, struct OsAddressSpaceInfo *OutStat) {
    struct OsCallResult result = OsSystemCall(eOsCallAddressSpaceStat, (uintptr_t)Handle, (uintptr_t)OutStat, 0, 0);
    return result.Status;
}
