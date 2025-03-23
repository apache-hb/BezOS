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
    class Clock;

    struct System {
        vfs2::VfsRoot *vfs;
        SystemObjects *objects;
        SystemMemory *memory;
        Scheduler *scheduler;
        Clock *clock;
    };
}

/// @brief Services syscall requests from userspace.
///
/// Functions in this namespace are called by the syscall handler to service requests from userspace.
/// This includes sanitizing inputs before passing them to the kernel.
namespace um {
    OsStatus ReadPath(km::CallContext *context, OsPath user, vfs2::VfsPath *path);
    OsStatus VerifyBuffer(km::CallContext *context, OsBuffer buffer);
    OsStatus SelectOwningProcess(km::System *system, km::CallContext *context, OsProcessHandle handle, km::Process **result);

    // <bezos/facility/fs.h>
    OsCallResult NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/device.h>
    OsCallResult DeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/process.h>
    OsCallResult ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessCurrent(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/clock.h>
    OsCallResult ClockStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ClockTime(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ClockTicks(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
}
