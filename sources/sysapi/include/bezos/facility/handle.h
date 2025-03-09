#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

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
/// @return The status of the operation.
extern OsStatus OsHandleWait(OsHandle Handle, OsInstant Timeout);

#ifdef __cplusplus
}
#endif
