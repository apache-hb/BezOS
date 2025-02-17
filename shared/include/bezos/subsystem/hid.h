#pragma once

#include <bezos/handle.h>
#include <bezos/key.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_DEVICE_PS2_KEYBOARD "System\0Devices\0HID\0KEYBOARD0"

OS_DEFINE_GUID(kOsHidClassGuid, 0xdd4ece3c, 0xec81, 0x11ef, 0x8b71, 0xc761047b867e);

enum {
    eOsHidEventUnknown   = UINT8_C(0),
    eOsHidEventKeyDown   = UINT8_C(1),
    eOsHidEventKeyUp     = UINT8_C(2),
    eOsHidEventMouseMove = UINT8_C(3),
};

typedef uint8_t OsHidEventType;

struct OsHidDeviceInfo {
    OsUtf8Char Name[OS_DEVICE_NAME_MAX];
};

struct OsHidKeyEvent {
    OsKey Key;
    OsKeyModifier Modifiers;
};

struct OsHidMouseEvent {
    int32_t DeltaX;
    int32_t DeltaY;
};

union OsHidEventData {
    OsHidKeyEvent Key;
    OsHidMouseEvent MouseMove;
};

struct OsHidEvent {
    OsHidEventType Type;
    OsInstant Time;
    OsHidEventData Body;
};

#ifdef __cplusplus
}
#endif
