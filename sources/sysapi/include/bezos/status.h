#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsStatus;

enum OsStatusId {
    /// @brief The operation was successful.
    OsStatusSuccess = 0x0,

    /// @brief The operation could not be completed due to a lack of memory.
    OsStatusOutOfMemory = 0x1,

    /// @brief The requested resource could not be found.
    OsStatusNotFound = 0x2,

    /// @brief The input to the operation was invalid.
    OsStatusInvalidInput = 0x3,

    /// @brief The resource does not support the operation.
    OsStatusNotSupported = 0x4,

    /// @brief The resource already exists.
    OsStatusAlreadyExists = 0x5,

    /// @brief Attempted to traverse over a non-folder inode.
    ///
    /// When walking a path an inode that wasnt a folder was encountered.
    OsStatusTraverseNonFolder = 0x6,

    /// @brief Attempted to perform an operation on an inode of the wrong type.
    ///
    /// When performing an operation on an inode the type of the inode
    /// was not what was expected.
    OsStatusInvalidType = 0x7,

    /// @brief Attempted to remove an inode that was not ready for removal.
    ///
    /// INodes with outstanding exclusive locks cannot be removed.
    OsStatusHandleLocked = 0x8,

    /// @brief The path is malformed.
    ///
    /// Malformed paths are paths that contain invalid characters, empty segments,
    /// or leading/trailing separators.
    OsStatusInvalidPath = 0x9,

    /// @brief An invalid syscall was invoked.
    OsStatusInvalidFunction = 0xa,

    /// @brief The end of the file was reached.
    OsStatusEndOfFile = 0xb,

    /// @brief The data is invalid.
    ///
    /// Data required for the operation is invalid, distinct from @ref OsStatusInvalidInput.
    /// This status is used for when a resource is in an invalid state, rather than the input
    /// parameters being invalid.
    OsStatusInvalidData = 0xc,

    /// @brief The version is invalid.
    OsStatusInvalidVersion = 0xd,

    /// @brief The operation timed out.
    OsStatusTimeout = 0xe,

    OsStatusOutOfBounds = 0xf,

    OsStatusMoreData = 0x10,
};

#ifdef __cplusplus
}
#endif
