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

    /// @brief Creation disposition flags.
    OsDeviceCreateFlags Flags;

    /// @brief The process to associate the device handle with.
    /// @note If this is @a OS_HANDLE_INVALID, then the device handle is created in the current process.
    OsProcessHandle Process;

    /// @brief Data to pass to the device during creation.
    const void *OpenData;

    /// @brief The size of the data to pass to the device creation.
    OsSize OpenDataSize;
};

struct OsDeviceReadRequest {
    void *BufferFront;
    void *BufferBack;
    uint64_t Offset;
    OsInstant Timeout;
};

struct OsDeviceWriteRequest {
    const void *BufferFront;
    const void *BufferBack;
    uint64_t Offset;
    OsInstant Timeout;
};

struct OsDeviceInfo {
    OsUtf8Char Name[OS_DEVICE_NAME_MAX];
    struct OsGuid InterfaceGuid;
    OsNodeHandle Node;
};

extern OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsAnyPointer Data, OsSize DataSize, OsDeviceHandle *OutHandle);

extern OsStatus OsDeviceClose(OsDeviceHandle Handle);

extern OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead);

extern OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten);

extern OsStatus OsDeviceInvoke(OsDeviceHandle Handle, uint64_t Function, void *Data, OsSize DataSize);

extern OsStatus OsDeviceStat(OsDeviceHandle Handle, struct OsDeviceInfo *OutInfo);

#ifdef __cplusplus
}
#endif
