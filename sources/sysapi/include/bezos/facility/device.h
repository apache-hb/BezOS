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
    /// @brief The name of the vnode that the device is associated with.
    OsUtf8Char Name[OS_DEVICE_NAME_MAX];

    /// @brief The GUID of the interface that the device implements.
    struct OsGuid InterfaceGuid;

    /// @brief The vnode handle that the device is associated with.
    OsNodeHandle Node;
};

/// @brief Open a device.
///
/// Open a device in the filesystem, equivalent to opening a vnode and querying for an interface in one step.
///
/// @param CreateInfo The create information for the device.
/// @param Data Data to pass to the device during creation.
/// @param DataSize The size of the data to pass to the device creation.
/// @param OutHandle The device handle.
///
/// @return The status of the operation.
extern OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsAnyPointer Data, OsSize DataSize, OsDeviceHandle *OutHandle);

/// @brief Close a device handle.
///
/// Closes a device handle, the device will only be discarded when all device handles are closed.
///
/// @param Handle The device handle to close.
///
/// @return The status of the operation.
extern OsStatus OsDeviceClose(OsDeviceHandle Handle);

/// @brief Read data from a device.
///
/// @param Handle The device handle to read from.
/// @param Request The read request.
/// @param OutRead The number of bytes read.
///
/// @return The status of the operation.
extern OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead);

/// @brief Write data to a device.
///
/// @param Handle The device handle to write to.
/// @param Request The write request.
/// @param OutWritten The number of bytes written.
///
/// @return The status of the operation.
extern OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten);

/// @brief Invoke a method on a device.
///
/// @param Handle The device handle to invoke the method on.
/// @param Function The function to invoke.
/// @param Data The data to pass to the function.
/// @param DataSize The size of the data to pass to the function.
///
/// @return The status of the operation.
extern OsStatus OsDeviceInvoke(OsDeviceHandle Handle, uint64_t Function, void *Data, OsSize DataSize);

/// @brief Query information about a device.
///
/// Queries information about a device, this can be used to get the node associated with the device.
///
/// @param Handle The device handle to query.
/// @param OutInfo The information about the device.
///
/// @return The status of the operation.
extern OsStatus OsDeviceStat(OsDeviceHandle Handle, struct OsDeviceInfo *OutInfo);

#ifdef __cplusplus
}
#endif
