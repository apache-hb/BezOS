#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Disposition flags for opening a file.
///
/// bits [0:7] contain the create mode.
/// bits [8:63] are reserved and must be zero.
typedef uint64_t OsDeviceCreateFlags;

enum {
    eOsDeviceOpenDefault = 0,

    /// @brief Open the device if it exists, otherwise fail.
    eOsDeviceOpenExisting = 0x0,

    /// @brief Create the device if it does not exist.
    eOsDeviceCreateNew = 0x1,

    /// @brief Create the device if it does not exist, otherwise open it.
    eOsDeviceCreateAlways = 0x2,
};

struct OsDeviceCreateInfo {
    /// @brief Path to the device.
    struct OsPath Path;

    /// @brief The interface GUID to query for.
    struct OsGuid InterfaceGuid;

    /// @brief The class GUID to use if creating a new device.
    struct OsGuid ClassGuid;

    /// @brief Creation disposition flags.
    OsDeviceCreateFlags Flags;

    /// @brief The process to associate the device handle with.
    /// @note If this is @a OS_HANDLE_INVALID, then the device handle is created in the current process.
    OsProcessHandle Process;
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
