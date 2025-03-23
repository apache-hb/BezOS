#include <bezos/facility/device.h>

#include <bezos/private.h>

OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsDeviceHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceOpen, (uintptr_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsDeviceHandle)result.Value;
    return result.Status;
}

OsStatus OsDeviceClose(OsDeviceHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceClose, (uintptr_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceRead, (uintptr_t)Handle, (uintptr_t)&Request, 0, 0);
    *OutRead = (OsSize)result.Value;
    return result.Status;
}

OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceWrite, (uintptr_t)Handle, (uintptr_t)&Request, 0, 0);
    *OutWritten = (OsSize)result.Value;
    return result.Status;
}

OsStatus OsDeviceInvoke(OsDeviceHandle Handle, uint64_t Function, void *Data, OsSize DataSize) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceInvoke, (uintptr_t)Handle, Function, (uintptr_t)Data, DataSize);
    return result.Status;
}

OsStatus OsDeviceStat(OsDeviceHandle Handle, struct OsDeviceInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceStat, (uintptr_t)Handle, (uintptr_t)OutInfo, 0, 0);
    return result.Status;
}
