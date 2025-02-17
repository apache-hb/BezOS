#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsThread Thread
/// @{

enum {
    eOsThreadNone = 0,

    eOsThreadSuspended = (1 << 0),
};

typedef uint32_t OsThreadCreateFlags;

struct OsThreadCreateInfo {
    const char *NameFront;
    const char *NameBack;

    OsProcessHandle Process;
    OsSize StackSize;
    OsAnyPointer EntryPoint;
    OsAnyPointer Argument;
    OsThreadCreateFlags Flags;
};

extern OsStatus OsThreadCurrent(OsThreadHandle *OutHandle);

extern OsStatus OsThreadYield(void);

extern OsStatus OsThreadSleep(OsDuration Duration);

extern OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle);

extern OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend);

extern OsStatus OsThreadDestroy(OsThreadHandle Handle);

/// @} // group OsThread

/// @defgroup OsMutex Mutex
/// @{

struct OsMutexCreateInfo {
    const char *NameFront;
    const char *NameBack;
};

extern OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle);

extern OsStatus OsMutexDestroy(OsMutexHandle Handle);

extern OsStatus OsMutexLock(OsMutexHandle Handle);

extern OsStatus OsMutexUnlock(OsMutexHandle Handle);

/// @} // group OsMutex

#ifdef __cplusplus
}
#endif
