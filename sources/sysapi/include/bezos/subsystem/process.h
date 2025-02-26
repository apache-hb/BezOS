#pragma once

#include <bezos/handle.h>

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

OS_DEFINE_GUID(kOsProcessGuid, 0xbef63de4, 0xf474, 0x11ef, 0x81ad, 0xd7f28f5e9282);

typedef uint64_t OsProcessId;

struct OsProcessInfo {
    OsProcessId Id;
    OsInstant StartTime;
    struct OsGuid User;
};

#ifdef __cplusplus
}
#endif
