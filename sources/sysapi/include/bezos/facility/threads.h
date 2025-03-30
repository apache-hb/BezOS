#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsThread Thread
/// @{

enum {
    eOsThreadAccessNone      = 0,
    eOsThreadAccessWait      = (1 << 0),
    eOsThreadAccessSuspend   = (1 << 1),
    eOsThreadAccessQuota     = (1 << 2),
    eOsThreadAccessTerminate = (1 << 3),

    eOsThreadAccessAll
        = eOsThreadAccessWait
        | eOsThreadAccessSuspend
        | eOsThreadAccessQuota
        | eOsThreadAccessTerminate,
};

enum {
    /// @brief The thread is immediately available for scheduling.
    eOsThreadCreateDefault = 0,

    /// @brief Create the thread in a suspended state.
    /// The thread will not be scheduled until it is resumed.
    eOsThreadCreateSuspended = (1 << 0),
};

enum {
    /// @brief Thread has exited.
    eOsThreadFinished  = 0,

    /// @brief Thread is currently running.
    eOsThreadRunning   = 1,

    /// @brief The thread has been manually suspended.
    eOsThreadSuspended = 2,

    /// @brief The thread is waiting for a handle to be signaled.
    eOsThreadWaiting   = 3,

    /// @brief Thread is not currently running, but is scheduled to run.
    eOsThreadQueued    = 4,

    /// @brief The process this thread is associated with has exited.
    eOsThreadOrphaned = 5,
};

typedef uint32_t OsThreadCreateFlags;
typedef uint32_t OsThreadState;

typedef void (*OsThreadEntry)(OsThreadHandle CurrentThread, OsAnyPointer Argument);

struct OsThreadCreateInfo {
    const char *NameFront;
    const char *NameBack;

    /// @brief The process this thread is associated with.
    OsProcessHandle Process;

    /// @brief The minimum size of the thread's stack.
    OsSize StackSize;

    /// @brief The entry point for the thread.
    OsThreadEntry EntryPoint;

    /// @brief The argument to pass to the thread's entry point.
    OsAnyPointer Argument;

    /// @brief Flags to control the creation of the thread.
    OsThreadCreateFlags Flags;
};

struct OsThreadInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];

    OsProcessHandle Process;
    OsThreadState State;
};

extern OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle);

extern OsStatus OsThreadDestroy(OsThreadHandle Handle);

extern OsStatus OsThreadCurrent(OsThreadHandle *OutHandle);

extern OsStatus OsThreadYield(void);

extern OsStatus OsThreadSleep(OsDuration Duration);

extern OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend);

extern OsStatus OsThreadStat(OsThreadHandle Handle, struct OsThreadInfo *OutInfo);

/// @} // group OsThread

/// @defgroup OsMutex Mutex
/// @{

struct OsMutexCreateInfo {
    const char *NameFront;
    const char *NameBack;

    OsProcessHandle Process;
};

extern OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle);

extern OsStatus OsMutexDestroy(OsMutexHandle Handle);

extern OsStatus OsMutexLock(OsMutexHandle Handle);

extern OsStatus OsMutexUnlock(OsMutexHandle Handle);

/// @} // group OsMutex

#ifdef __cplusplus
}
#endif
