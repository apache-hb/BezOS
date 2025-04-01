#include <bezos/facility/device.h>

#include <bezos/private.h>

OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsDeviceHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceOpen, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsDeviceHandle)result.Value;
    return result.Status;
}

OsStatus OsDeviceClose(OsDeviceHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceClose, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceRead, (uint64_t)Handle, (uint64_t)&Request, 0, 0);
    *OutRead = (OsSize)result.Value;
    return result.Status;
}

OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceWrite, (uint64_t)Handle, (uint64_t)&Request, 0, 0);
    *OutWritten = (OsSize)result.Value;
    return result.Status;
}

OsStatus OsDeviceInvoke(OsDeviceHandle Handle, uint64_t Function, void *Data, OsSize DataSize) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceInvoke, (uint64_t)Handle, Function, (uint64_t)Data, DataSize);
    return result.Status;
}

OsStatus OsDeviceStat(OsDeviceHandle Handle, struct OsDeviceInfo *OutInfo) {
    struct OsCallResult result = OsSystemCall(eOsCallDeviceStat, (uint64_t)Handle, (uint64_t)OutInfo, 0, 0);
    return result.Status;
}
