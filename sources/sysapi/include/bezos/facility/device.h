#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsDeviceCreateFlags;

enum {
    eOsDeviceNone = 0,

    /// @brief Create the device using the default interface if the device does not exist.
    eOsDeviceCreateNew = (1 << 0),

    /// @brief Open the device only if it exists.
    eOsDeviceOpenAlways = (1 << 1),
};

struct OsDeviceCreateInfo {
    struct OsPath Path;

    struct OsGuid InterfaceGuid;

    OsDeviceCreateFlags Flags;
};

struct OsDeviceReadRequest {
    void *BufferFront;
    void *BufferBack;
    OsInstant Timeout;
};

struct OsDeviceWriteRequest {
    const void *BufferFront;
    const void *BufferBack;
    OsInstant Timeout;
};

struct OsDeviceCallRequest {
    uint64_t Function;
    const void *InputFront;
    const void *InputBack;

    void *OutputFront;
    void *OutputBack;
};

extern OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsAnyPointer Data, OsSize DataSize, OsDeviceHandle *OutHandle);

extern OsStatus OsDeviceClose(OsDeviceHandle Handle);

extern OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead);

extern OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten);

extern OsStatus OsDeviceCall(OsDeviceHandle Handle, uint64_t Function, void *Data, OsSize DataSize);

#ifdef __cplusplus
}
#endif
