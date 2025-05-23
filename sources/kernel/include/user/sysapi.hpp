#pragma once

#include <bezos/private.h>
#include <bezos/handle.h>

namespace vfs {
    class VfsRoot;
    class VfsPath;
}

namespace sys {
    class System;
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
        vfs::VfsRoot *vfs;
        SystemMemory *memory;
        Clock *clock;
        sys::System *sys;
    };
}

/// @brief Services syscall requests from userspace.
///
/// Functions in this namespace are called by the syscall handler to service requests from userspace.
/// This includes sanitizing inputs before passing them to the kernel.
namespace um {
    static constexpr bool kUseNewSystem = true;

    OsStatus ReadPath(km::CallContext *context, OsPath user, vfs::VfsPath *path);
    OsStatus VerifyBuffer(km::CallContext *context, OsBuffer buffer);
    OsStatus SelectOwningProcess(km::System *system, km::CallContext *context, OsProcessHandle handle, km::Process **result);

    // <bezos/facility/clock.h>
    OsCallResult ClockStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ClockTime(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ClockTicks(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/device.h>
    OsCallResult DeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult DeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/handle.h>
    OsCallResult HandleWait(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult HandleClone(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult HandleClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult HandleStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult HandleOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/mutex.h>
    OsCallResult MutexCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult MutexDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult MutexLock(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult MutexUnlock(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/node.h>
    OsCallResult NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/process.h>
    OsCallResult ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessCurrent(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/thread.h>
    OsCallResult ThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ThreadDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ThreadSleep(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult ThreadSuspend(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);

    // <bezos/facility/vmem.h>
    OsCallResult VmemCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult VmemMap(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
    OsCallResult VmemDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs);
}
