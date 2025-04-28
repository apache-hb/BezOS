#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t OsFacility;

enum OsFacilityId {
    eOsFacilityUnknown = 0x00,
    eOsFacilityMemory  = 0x01,
    eOsFacilityVmem    = 0x02,
};

typedef uint64_t OsStatus;

enum OsStatusId {
    /// @brief The operation was successful.
    OsStatusSuccess = 0x0000,

    /// @brief The operation could not be completed due to a lack of memory.
    OsStatusOutOfMemory = 0x0001,

    /// @brief The requested resource could not be found.
    OsStatusNotFound = 0x0002,

    /// @brief The input to the operation was invalid.
    OsStatusInvalidInput = 0x0003,

    /// @brief The resource does not support the operation.
    OsStatusNotSupported = 0x0004,

    /// @brief The resource already exists.
    OsStatusAlreadyExists = 0x0005,

    /// @brief Attempted to traverse over a non-folder inode.
    ///
    /// When walking a path an inode that wasnt a folder was encountered.
    OsStatusTraverseNonFolder = 0x0006,

    /// @brief Attempted to perform an operation on an inode of the wrong type.
    ///
    /// When performing an operation on an inode the type of the inode
    /// was not what was expected.
    OsStatusInvalidType = 0x0007,

    /// @brief Attempted to remove an inode that was not ready for removal.
    ///
    /// INodes with outstanding exclusive locks cannot be removed.
    OsStatusHandleLocked = 0x0008,

    /// @brief The path is malformed.
    ///
    /// Malformed paths are paths that contain invalid characters, empty segments,
    /// or leading/trailing separators.
    OsStatusInvalidPath = 0x0009,

    /// @brief An invalid syscall was invoked.
    OsStatusInvalidFunction = 0x000a,

    /// @brief The end of the file was reached.
    OsStatusEndOfFile = 0x000b,

    /// @brief The data is invalid.
    ///
    /// Data required for the operation is invalid, distinct from @ref OsStatusInvalidInput.
    /// This status is used for when a resource is in an invalid state, rather than the input
    /// parameters being invalid.
    OsStatusInvalidData = 0x000c,

    /// @brief The version is invalid.
    OsStatusInvalidVersion = 0x000d,

    /// @brief The operation timed out.
    OsStatusTimeout = 0x000e,

    OsStatusOutOfBounds = 0x000f,

    /// @brief There is more data available.
    OsStatusMoreData = 0x0010,

    OsStatusChecksumError = 0x0011,

    /// @brief The handle is stale or invalid.
    OsStatusInvalidHandle = 0x0012,

    /// @brief The memory address is not available.
    OsStatusInvalidAddress = 0x0013,

    /// @brief The memory span specified is invalid.
    OsStatusInvalidSpan = 0x0014,

    /// @brief The device has misbehaved.
    OsStatusDeviceFault = 0x0015,

    OsStatusDeviceBusy = 0x0016,

    OsStatusDeviceNotReady = 0x0017,

    /// @brief The node does not support the requested interface.
    OsStatusInterfaceNotSupported = 0x0018,

    /// @brief The interface does not support the requested function.
    OsStatusFunctionNotSupported  = 0x0019,

    /// @brief The function does not support the requested operation.
    OsStatusOperationNotSupported = 0x001a,

    /// @brief The operation was completed.
    OsStatusCompleted = 0x001b,

    /// @brief The operation was denied due to insufficient permissions.
    OsStatusAccessDenied = 0x001c,

    /// @brief The parent process has exited and this process is now orphaned.
    OsStatusProcessOrphaned = 0x001d,

    /// @brief The requested resource was found, but is not available.
    OsStatusNotAvailable = 0x001e,
};

#define OS_SUCCESS(status) ((status) == OsStatusSuccess)
#define OS_ERROR(status) ((status) != OsStatusSuccess)

#ifdef __cplusplus
}
#endif
