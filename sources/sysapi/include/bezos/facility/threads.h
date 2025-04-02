#pragma once

#include <bezos/arch/arch.h>
#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsThread Thread
/// @{

enum {
    eOsThreadAccessNone      = 0,

    eOsThreadAccessWait      = (1 << 0),
    eOsThreadAccessTerminate = (1 << 2),
    eOsThreadAccessStat      = (1 << 3),

    eOsThreadAccessSuspend   = (1 << 4),
    eOsThreadAccessQuota     = (1 << 5),

    eOsThreadAccessAll
        = eOsThreadAccessWait
        | eOsThreadAccessTerminate
        | eOsThreadAccessStat
        | eOsThreadAccessSuspend
        | eOsThreadAccessQuota,
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

struct OsThreadCreateInfo {
    const char *NameFront;
    const char *NameBack;

    struct OsMachineContext CpuState;

    /// @brief Flags to control the creation of the thread.
    OsThreadCreateFlags Flags;

    /// @brief The process this thread is associated with.
    OsProcessHandle Process;

    OsTxHandle Transaction;
};

struct OsThreadInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];

    struct OsMachineContext CpuState;

    OsThreadState State;

    OsProcessHandle Process;
};

extern OsStatus OsThreadCurrent(OsHandleAccess Access, OsThreadHandle *OutHandle);

extern OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle);

extern OsStatus OsThreadDestroy(OsThreadHandle Handle);

extern OsStatus OsThreadYield(void);

extern OsStatus OsThreadSleep(OsDuration Duration);

extern OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend);

extern OsStatus OsThreadStat(OsThreadHandle Handle, struct OsThreadInfo *OutInfo);

/// @} // group OsThread

#ifdef __cplusplus
}
#endif
