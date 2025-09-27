#include "system/create.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "syscall.hpp"

static OsCallResult NewProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsProcessCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    OsProcessHandle handle = OS_HANDLE_INVALID;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysProcessCreate(&invoke, createInfo, &handle)) {
        return km::CallError(status);
    }

    return km::CallOk(handle);
}

static OsCallResult NewProcessDestroy(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userProcess = regs->arg0;
    uint64_t userExitCode = regs->arg1;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysProcessDestroy(&invoke, userProcess, userExitCode, eOsProcessExited)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewProcessCurrent(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userAccess = regs->arg0;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    if (OsStatus status = sys::SysProcessCurrent(&invoke, userAccess, &handle)) {
        return km::CallError(status);
    }

    return km::CallOk(handle);
}

static OsCallResult NewProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    if (!context->isMapped(userStat, userStat + sizeof(OsProcessInfo), km::PageFlags::eUser | km::PageFlags::eWrite)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };

    if (OsStatus status = sys::SysProcessStat(&invoke, userHandle, (OsProcessInfo*)userStat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::ProcessCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewProcessCreate(system, context, regs);
}

OsCallResult um::ProcessDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewProcessDestroy(system, context, regs);
}

OsCallResult um::ProcessCurrent(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewProcessCurrent(system, context, regs);
}

OsCallResult um::ProcessStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewProcessStat(system, context, regs);
}
