#include "user/sysapi.hpp"

#include "system/system.hpp"

#include "system/invoke.hpp"
#include "system/schedule.hpp"

#include "syscall.hpp"

OsCallResult um::HandleWait(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::HandleClone(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::HandleClose(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };

    if (OsStatus status = sys::SysHandleClose(&invoke, userHandle)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::HandleStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    if (!context->isMapped(userStat, userStat + sizeof(OsHandleInfo), km::PageFlags::eUser | km::PageFlags::eWrite)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysHandleStat(&invoke, userHandle, (OsHandleInfo*)userStat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::HandleOpen(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}
