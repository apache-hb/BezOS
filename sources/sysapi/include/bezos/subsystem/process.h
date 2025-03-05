#pragma once

#include <bezos/handle.h>

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

OS_DEFINE_GUID(kOsProcessGuid, 0xbef63de4, 0xf474, 0x11ef, 0x81ad, 0xd7f28f5e9282);

typedef uint64_t OsProcessId;

enum {
    eOsProcessInfo = UINT64_C(0),
};

struct OsProcessInfo {
    OsProcessId Id;
    OsProcessId ParentId;
    OsInstant StartTime;
    struct OsGuid User;
};

inline OsStatus OsInvokeProcessGetInfo(OsDeviceHandle Handle, struct OsProcessInfo *OutInfo) {
    return OsDeviceCall(Handle, eOsProcessInfo, OutInfo, sizeof(struct OsProcessInfo));
}

#define OS_DEVICE_PROCESS_LIST "Processes"

OS_DEFINE_GUID(kOsTaskListGuid, 0x0e36758a, 0xf70a, 0x11ef, 0x9b0c, 0x0b306adc6cfa);

enum {
    eOsProcessListGet = UINT64_C(0),
};

struct OsProcessListData {
    uint32_t Count;
    OsProcessId Processes[];
};

inline OsStatus OsInvokeProcessListGet(OsDeviceHandle Handle, struct OsProcessListData *OutData) {
    return OsDeviceCall(Handle, eOsProcessListGet, OutData, sizeof(struct OsProcessListData));
}

#ifdef __cplusplus
}
#endif
