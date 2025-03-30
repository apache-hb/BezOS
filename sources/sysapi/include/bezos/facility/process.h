#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsProcess Process
/// @{

OS_DEFINE_GUID(kOsCommandLineParamGuid, 0x768582ba, 0x0197, 0x11f0, 0xbc84, 0x7f8e0b26baac);

enum {
    eOsProcessAccessNone = 0,

    /// @brief Grants access to wait for the process to exit.
    eOsProcessAccessWait          = (1 << 0),

    /// @brief Grants access to terminate the process.
    eOsProcessAccessTerminate     = (1 << 1),

    /// @brief Grants access to suspend and resume the process.
    eOsProcessAccessSuspend       = (1 << 2),

    /// @brief Grants access to create, update, and destroy virtual memory mappings.
    eOsProcessAccessVmControl     = (1 << 3),

    /// @brief Grants access to create, update, and destroy threads.
    eOsProcessAccessThreadControl = (1 << 4),

    /// @brief Grants access to create, update, and destroy devices and nodes.
    eOsProcessAccessIoControl     = (1 << 5),

    /// @brief Grants access to create, update, and destroy child processes.
    eOsProcessAccessProcessControl = (1 << 6),

    /// @brief Grants access to create, update, and destroy quotas.
    eOsProcessAccessQuota         = (1 << 7),

    eOsProcessAccessAll
        = eOsProcessAccessWait
        | eOsProcessAccessTerminate
        | eOsProcessAccessSuspend
        | eOsProcessAccessVmControl
        | eOsProcessAccessThreadControl
        | eOsProcessAccessIoControl
        | eOsProcessAccessProcessControl
        | eOsProcessAccessQuota,
};

enum {
    eOsProcessNone = 0,

    /// @brief Process is running.
    eOsProcessRunning    = 0x0,

    /// @brief Process has been suspended, either by itself or by another process.
    eOsProcessSuspended  = 0x1,

    /// @brief Process exited normally.
    eOsProcessExited     = 0x2,

    /// @brief Process exited due to an unhandled fault.
    eOsProcessFaulted    = 0x3,

    /// @brief Process was killed by the operating system.
    eOsProcessTerminated = 0x4,

    /// @brief Process was orphaned due to a parent process exiting.
    eOsProcessOrphaned    = 0x5,

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

struct OsProcessParamHeader {
    struct OsGuid Guid;
    uint32_t DataSize;
};

/// @brief A parameter passed to a process at creation time
struct OsProcessParam {
    struct OsGuid Guid;
    uint32_t DataSize;
    OsByte Data[];
};

struct OsProcessCreateInfo {
    struct OsPath Executable;

    const char *NameBegin;
    const char *NameEnd;

    const struct OsProcessParam *ArgsBegin;
    const struct OsProcessParam *ArgsEnd;

    OsProcessStateFlags Flags;
    OsProcessHandle Parent;
};

struct OsProcessInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];

    OsProcessHandle Parent;

    const struct OsProcessParam *ArgsBegin;
    const struct OsProcessParam *ArgsEnd;

    OsProcessStateFlags Status;
    int64_t ExitCode;
};

extern OsStatus OsProcessCurrent(OsProcessHandle *OutHandle);

/// @brief Create a new process and return a handle to it
///
/// Creates an empty process object and initializes it with the given parameters.
/// The new process has no threads, an empty virtual address space, and no open handles.
///
/// @note The returned handle is granted @a eOsProcessAccessAll access to the process object.
///
/// @param CreateInfo The parameters to use when creating the process.
/// @param OutHandle The handle to the new process object.
///
/// @return An error code indicating the result of the operation.
extern OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle);

extern OsStatus OsProcessTerminate(OsProcessHandle Handle, int64_t ExitCode);

extern OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend);

extern OsStatus OsProcessStat(OsProcessHandle Handle, struct OsProcessInfo *OutState);

/// @} // group OsProcess

#ifdef __cplusplus
}
#endif
