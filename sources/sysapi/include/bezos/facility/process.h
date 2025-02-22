#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsProcess Process
/// @{

enum {
    eOsProcessNone = 0,

    eOsProcessSuspended = (1 << 0),
    eOsProcessSupervisor = (1 << 1),
};

typedef uint32_t OsProcessCreateFlags;

struct OsProcessCreateInfo {
    const char *NameFront;
    const char *NameBack;

    const char *ArgumentsBegin;
    const char *ArgumentsEnd;

    OsProcessCreateFlags Flags;
    OsProcessHandle Parent;
};

extern OsStatus OsProcessCurrent(OsProcessHandle *OutHandle);

extern OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle);

extern OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend);

extern OsStatus OsProcessTerminate(OsProcessHandle Handle);

/// @} // group OsProcess

#ifdef __cplusplus
}
#endif
