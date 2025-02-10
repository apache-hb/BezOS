#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsStatus;

enum OsStatusId {
    OsStatusSuccess = 0,

    OsStatusOutOfMemory = 1,

    OsStatusNotFound = 2,

    OsStatusInvalidInput = 3,

    OsStatusNotSupported = 4,

    OsStatusAlreadyExists = 5,

    /// @brief Attempted to traverse over a non-folder inode.
    ///
    /// When walking a path an inode that wasnt a folder
    /// was encountered.
    OsStatusTraverseNonFolder = 6,

    /// @brief Attempted to perform an operation on an inode of the wrong type.
    ///
    /// When performing an operation on an inode the type of the inode
    /// was not what was expected.
    OsStatusInvalidType = 7,

    /// @brief Attempted to remove an inode that was not ready for removal.
    ///
    /// INodes with outstanding exclusive locks cannot be removed.
    OsStatusHandleLocked = 8,

    /// @brief The path is malformed.
    ///
    /// Malformed paths are paths that contain invalid characters, empty segments,
    /// or leading/trailing separators.
    OsStatusInvalidPath = 9,
};

inline bool OsStatusOk(OsStatus status) {
    return status == OsStatusSuccess;
}

inline bool OsStatusError(OsStatus status) {
    return !OsStatusOk(status);
}

#ifdef __cplusplus
}
#endif
