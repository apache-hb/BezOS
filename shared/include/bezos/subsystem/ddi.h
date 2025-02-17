#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_DEVICE_FRAMEBUFFER "System\0Devices\0Display\0RAMFB0"

OS_DEFINE_GUID(kOsDisplayClassGuid, 0x45a46f76, 0xed6a, 0x11ef, 0x8516, 0x33da61d08982);

#ifdef __cplusplus
}
#endif
