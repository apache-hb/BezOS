#pragma once

#include <bezos/handle.h>
#include <bezos/key.h>

#ifdef __cplusplus
extern "C" {
#endif

OS_DEFINE_GUID(kOsHidClassGuid, 0xdd4ece3c, 0xec81, 0x11ef, 0x8b71, 0xc761047b867e);

enum {
    eOsHidDeviceClassUnknown  = UINT8_C(0),
    eOsHidDeviceClassKeyboard = UINT8_C(1),
    eOsHidDeviceClassMouse    = UINT8_C(2),
    eOsHidDeviceClassGamepad  = UINT8_C(3),
};

enum {
    eOsHidEventUnknown   = UINT8_C(0),
    eOsHidEventKeyDown   = UINT8_C(1),
    eOsHidEventKeyUp     = UINT8_C(2),
    eOsHidEventMouseMove = UINT8_C(3),
};

typedef uint8_t OsHidDeviceClass;
typedef uint8_t OsHidEventType;

struct OsHidDeviceInfo {
    OsHidDeviceClass DeviceClass;
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
