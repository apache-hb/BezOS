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
    struct OsPath Executable;
    struct OsPath WorkingDirectory;

    const char *ArgumentsBegin;
    const char *ArgumentsEnd;

    OsProcessCreateFlags Flags;
    OsProcessHandle Parent;
};

extern OsStatus OsProcessCurrent(OsProcessHandle *OutHandle);

extern OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle);

extern OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend);

extern OsStatus OsProcessTerminate(OsProcessHandle Handle, int64_t ExitCode);

/// @} // group OsProcess

#ifdef __cplusplus
}
#endif
