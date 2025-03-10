#pragma once

#include <bezos/handle.h>

OS_BEGIN_API

OS_DEFINE_GUID(kOsFileGuid, 0x538202b8, 0xf97a, 0x11ef, 0x9446, 0x43376bcec51c);
OS_DEFINE_GUID(kOsFolderGuid, 0x5382039e, 0xf97a, 0x11ef, 0x9447, 0x73b093d39f67);
OS_DEFINE_GUID(kOsStreamGuid, 0xa2b6183e, 0xf9e2, 0x11ef, 0x9bf1, 0xdb0e14965a6f);
OS_DEFINE_GUID(kOsSocketGuid, 0x7e74cd3a, 0xfd07, 0x11ef, 0x859a, 0x7faf25edb7ab);

struct OsStreamCreateInfo {
    struct OsPath Path;
};

extern OsStatus OsStreamCreate(struct OsStreamCreateInfo CreateInfo, OsDeviceHandle *OutStream);

OS_END_API
