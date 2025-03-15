#pragma once

#include <bezos/private.h>
#include <bezos/handle.h>

namespace vfs2 {
    class VfsRoot;
    class VfsPath;
}

namespace km {
    struct Process;

    class CallContext;
    struct SystemCallRegisterSet;
    class SystemObjects;
    class SystemMemory;
    class Scheduler;

    struct System {
        vfs2::VfsRoot *vfs;
        SystemObjects *objects;
        SystemMemory *memory;
        Scheduler *scheduler;
    };
}

/// @brief Services syscall requests from userspace.
///
/// Functions in this namespace are called by the syscall handler to service requests from userspace.
/// This includes sanitizing inputs before passing them to the kernel.
namespace um {
    OsStatus UserReadPath(km::CallContext *context, OsPath user, vfs2::VfsPath *path);
    OsStatus SelectOwningProcess(km::System *system, km::CallContext *context, OsProcessHandle handle, km::Process **result);

    OsCallResult NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    OsCallResult DeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    OsCallResult ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessCurrent(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
}
