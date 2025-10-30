#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsMutex Mutex
/// @{

enum {
    eOsMutexAccessNone = eOsAccessNone,

    eOsMutexAccessRead = eOsAccessRead,
    eOsMutexAccessWrite = eOsAccessWrite,
    eOsMutexAccessStat = eOsAccessStat,
    eOsMutexAccessDestroy = eOsAccessDestroy,
    eOsMutexAccessWait = eOsAccessWait,

    eOsMutexAccessLock = OS_IMPL_ACCESS_BIT(0),
    eOsMutexAccessLockShared = OS_IMPL_ACCESS_BIT(1),

    eOsMutexAccessAll
        = eOsMutexAccessRead
        | eOsMutexAccessWrite
        | eOsMutexAccessStat
        | eOsMutexAccessDestroy
        | eOsMutexAccessWait
        | eOsMutexAccessLock
        | eOsMutexAccessLockShared,
};

typedef OsHandleAccess OsMutexAccess;

struct OsMutexCreateInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];

    const char *NameFront;
    const char *NameBack;

    OsProcessHandle Process;

    OsTxHandle Transaction;
};

struct OsMutexInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];
};

extern OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle);

extern OsStatus OsMutexDestroy(OsMutexHandle Handle);

extern OsStatus OsMutexLock(OsMutexHandle Handle, OsInstant Timeout);

extern OsStatus OsMutexUnlock(OsMutexHandle Handle);

extern OsStatus OsMutexLockShared(OsMutexHandle Handle, OsInstant Timeout);

extern OsStatus OsMutexUnlockShared(OsMutexHandle Handle);

extern OsStatus OsMutexStat(OsMutexHandle Handle, struct OsMutexInfo *OutInfo);

/// @} // group OsMutex

#ifdef __cplusplus
}
#endif
