#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsProcess Process
/// @{

OS_DEFINE_GUID(kOsCommandLineParamGuid, 0x768582ba, 0x0197, 0x11f0, 0xbc84, 0x7f8e0b26baac);

enum {
    eOsProcessNone = 0,

    /// @brief Process is running
    eOsProcessRunning    = 0x0,

    /// @brief Process has been suspended, either by itself or by another process
    eOsProcessSuspended  = 0x1,

    /// @brief Process exited normally
    eOsProcessExited     = 0x2,

    /// @brief Process exited due to an unhandled fault
    eOsProcessFaulted    = 0x3,

    eOsProcessStatusMask = 0xF,



    /// @brief Process is running with usermode privileges
    eOsProcessUser       = (0x0 << 8),

    /// @brief Process is running with supervisor privileges
    eOsProcessSupervisor = (0x1 << 8),

    eOsProcessPrivilegeMask = 0xF00,
};

#define OS_PROCESS_STATUS(FLAGS) ((FLAGS) & eOsProcessStatusMask)
#define OS_PROCESS_PRIVILEGE(FLAGS) ((FLAGS) & eOsProcessPrivilegeMask)

/// @brief The state of a process
///
/// bits [0:3] - Status
/// bits [8:11] - Privilege
typedef uint32_t OsProcessStateFlags;

/// @brief A parameter passed to a process at creation time
struct OsProcessParam {
    struct OsGuid Guid;
    uint32_t DataSize;
    OsByte Data[];
};

struct OsProcessCreateInfo {
    struct OsPath Executable;
    struct OsPath WorkingDirectory;

    const struct OsProcessParam *ObjectParametersBegin;
    const struct OsProcessParam *ObjectParametersEnd;

    OsProcessStateFlags Flags;
    OsProcessHandle Parent;
};

struct OsProcessInfo {
    OsProcessStateFlags Status;
    int64_t ExitCode;
};

extern OsStatus OsProcessCurrent(OsProcessHandle *OutHandle);

extern OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle);

extern OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend);

extern OsStatus OsProcessTerminate(OsProcessHandle Handle, int64_t ExitCode);

extern OsStatus OsProcessStat(OsProcessHandle Handle, struct OsProcessInfo *OutState);

extern OsStatus OsProcessArg(OsProcessHandle Handle, struct OsGuid Guid, void *OutData, OsSize *OutSize);

/// @} // group OsProcess

#ifdef __cplusplus
}
#endif
