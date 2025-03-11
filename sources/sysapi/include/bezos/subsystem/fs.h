#pragma once

#include <bezos/handle.h>

OS_BEGIN_API

OS_DEFINE_GUID(kOsFileGuid, 0x538202b8, 0xf97a, 0x11ef, 0x9446, 0x43376bcec51c);
OS_DEFINE_GUID(kOsFolderGuid, 0x5382039e, 0xf97a, 0x11ef, 0x9447, 0x73b093d39f67);
OS_DEFINE_GUID(kOsStreamGuid, 0xa2b6183e, 0xf9e2, 0x11ef, 0x9bf1, 0xdb0e14965a6f);
OS_DEFINE_GUID(kOsSocketGuid, 0x7e74cd3a, 0xfd07, 0x11ef, 0x859a, 0x7faf25edb7ab);
OS_DEFINE_GUID(kOsTerminalGuid, 0x140706c7, 0xfe9a, 0x11ef, 0x8a0c, 0x17ca63280077);
OS_DEFINE_GUID(kOsHardLinkGuid, 0x14070816, 0xfe9a, 0x11ef, 0x8a0d, 0x432d3e3e7bb8);
OS_DEFINE_GUID(kOsSymbolicLinkGuid, 0x14070837, 0xfe9a, 0x11ef, 0x8a0e, 0x1387708268f2);

struct OsStreamCreateInfo {
    struct OsPath Path;
};

extern OsStatus OsStreamCreate(struct OsStreamCreateInfo CreateInfo, OsDeviceHandle *OutStream);

OS_END_API
