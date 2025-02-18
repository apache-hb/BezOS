#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_DEVICE_DDI_RAMFB "System\0Devices\0DDI\0RAMFB0"

OS_DEFINE_GUID(kOsDisplayClassGuid, 0x45a46f76, 0xed6a, 0x11ef, 0x8516, 0x33da61d08982);

enum {
    eOsDdiInfo = UINT64_C(0),
    eOsDdiBlit = UINT64_C(1),
    eOsDdiFill = UINT64_C(2),
};

struct OsDdiDisplayInfo {
    OsUtf8Char Name[OS_DEVICE_NAME_MAX];

    uint32_t Width;
    uint32_t Height;
    uint32_t Stride;

    uint8_t BitsPerPixel;

    uint8_t RedMaskSize;
    uint8_t RedMaskShift;

    uint8_t GreenMaskSize;
    uint8_t GreenMaskShift;

    uint8_t BlueMaskSize;
    uint8_t BlueMaskShift;
};

struct OsDdiBlit {
    const void *SourceFront;
    const void *SourceBack;
};

struct OsDdiFill {
    uint8_t R;
    uint8_t G;
    uint8_t B;
};

#ifdef __cplusplus
}
#endif
