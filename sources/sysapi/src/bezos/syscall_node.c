#include <bezos/facility/node.h>

#include <bezos/private.h>

//
// Node syscalls
//

OsStatus OsNodeOpen(struct OsNodeCreateInfo CreateInfo, OsNodeHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallNodeOpen, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsNodeHandle)result.Value;
    return result.Status;
}

OsStatus OsNodeClose(OsNodeHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallNodeClose, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsNodeQueryInterface(OsNodeHandle Handle, struct OsNodeQueryInterfaceInfo Query, OsDeviceHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallNodeQuery, (uint64_t)Handle, (uint64_t)&Query, 0, 0);
    *OutHandle = (OsDeviceHandle)result.Value;
    return result.Status;
}

OsStatus OsNodeStat(OsNodeHandle Handle, struct OsNodeInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallNodeStat, (uint64_t)Handle, (uint64_t)OutInfo, 0, 0);
    return result.Status;
}
