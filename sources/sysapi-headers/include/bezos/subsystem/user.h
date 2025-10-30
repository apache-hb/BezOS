#pragma once

#include <bezos/handle.h>

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

OS_DEFINE_GUID(kOsUserGuid, 0x516d10bc, 0xf475, 0x11ef, 0xa120, 0x4f23a297e81f);

struct OsUserInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];
};

#ifdef __cplusplus
}
#endif
