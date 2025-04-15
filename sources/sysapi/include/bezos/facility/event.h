#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    eOsEventAccessNone = 0,

    /// @brief Grants access to wait on the object.
    eOsEventAccessWait = (1 << 0),

    /// @brief Grants access to signal the object.
    eOsEventAccessSignal = (1 << 1),

    /// @brief Grants access to destroy the object.
    eOsEventAccessDestroy = (1 << 2),

    /// @brief Grants access to query the event info.
    eOsEventAccessStat = (1 << 3),

    eOsEventAccessAll
        = eOsEventAccessWait
        | eOsEventAccessSignal
        | eOsEventAccessDestroy
        | eOsEventAccessStat,
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
