#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsHandleCloneInfo {
    OsHandleAccess Access;
    OsProcessHandle Process;
    OsTxHandle Transaction;
};

struct OsHandleInfo {
    OsHandleAccess Access;
};

/// @brief Wait on a handle to become signaled.
///
/// All handles can be waited on, but the behavior of waiting on a handle is
/// specific to the handle type.
///
/// Waiting on a handle will block the calling thread until the handle is
/// signaled or the timeout expires.
///
/// Waiting on a process will wait for the process to exit.
/// Waiting on a thread will wait for the thread to exit.
///
/// @param Handle The handle to wait on.
/// @param Timeout The maximum time to wait for the handle to become signaled.
///                If the timeout is zero the function will return immediately.
///                If the timeout is OS_INFINITE the function will block indefinitely.
///
/// @return The status of the operation.
extern OsStatus OsHandleWait(OsHandle Handle, OsInstant Timeout);

/// @brief Clone a handle with new access rights.
///
/// Clone a handle with new access rights. If @p CloneInfo.Process is not @a OS_HANDLE_INVALID,
/// the new handle will be created in the context of the specified process. This requires the process
/// handle has @a eOsProcessAccessIoControl access.
///
/// @pre @p Handle must be a valid handle
/// @pre @p CloneInfo.Access must be a valid access mask for the handle type
/// @pre @p CloneInfo.Access must not be zero
/// @pre @p CloneInfo.Access must be a subset of the original handle's access rights
/// @pre @p OutHandle must be a valid pointer
/// @pre @p OutHandle must not be NULL
/// @pre @p OutHandle must not be the same as @p Handle
///
/// @param Handle The handle to clone.
/// @param CloneInfo The information for the clone.
/// @param OutHandle The new handle.
///
/// @return The status of the operation.
extern OsStatus OsHandleClone(OsHandle Handle, struct OsHandleCloneInfo CloneInfo, OsHandle *OutHandle);

extern OsStatus OsHandleClose(OsHandle Handle);

extern OsStatus OsHandleStat(OsHandle Handle, struct OsHandleInfo *OutInfo);

#ifdef __cplusplus
}
#endif
