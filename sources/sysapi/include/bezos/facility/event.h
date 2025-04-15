#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    eOsEventAccessNone = eOsAccessNone,

    eOsEventAccessRead = eOsAccessRead,
    eOsEventAccessWrite = eOsAccessWrite,
    eOsEventAccessStat = eOsAccessStat,
    eOsEventAccessDestroy = eOsAccessDestroy,
    eOsEventAccessWait = eOsAccessWait,

    /// @brief Grants access to signal the event.
    eOsEventAccessSignal = OS_IMPL_ACCESS_BIT(0),

    eOsEventAccessAll
        = eOsEventAccessRead
        | eOsEventAccessWrite
        | eOsEventAccessStat
        | eOsEventAccessDestroy
        | eOsEventAccessWait
        | eOsEventAccessSignal,
};

typedef OsHandleAccess OsEventAccess;

struct OsEventCreateInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];

    OsProcessHandle Process;

    OsTxHandle Transaction;
};

struct OsEventInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];
};

extern OsStatus OsEventCreate(struct OsEventCreateInfo CreateInfo, OsEventHandle *OutHandle);

extern OsStatus OsEventSignal(OsEventHandle Handle);
extern OsStatus OsEventStat(OsEventHandle Handle, struct OsEventInfo *OutInfo);
extern OsStatus OsEventClose(OsEventHandle Handle);

#ifdef __cplusplus
}
#endif
