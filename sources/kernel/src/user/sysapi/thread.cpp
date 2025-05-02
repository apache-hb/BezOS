#include "system/create.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "syscall.hpp"

static OsCallResult NewThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsThreadCreateInfo createInfo{};
    OsThreadHandle thread = OS_HANDLE_INVALID;

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysThreadCreate(&invoke, createInfo, &thread)) {
        return km::CallError(status);
    }

    return km::CallOk(thread);
}

static OsCallResult NewThreadDestroy(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userThread = regs->arg0;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysThreadDestroy(&invoke, eOsThreadFinished, userThread)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    sys::YieldCurrentThread();
    return km::CallOk(0zu);
}

OsCallResult um::ThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewThreadCreate(system, context, regs);
}

OsCallResult um::ThreadDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewThreadDestroy(system, context, regs);
}

OsCallResult um::ThreadSleep(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewThreadSleep(system, context, regs);
}

OsCallResult um::ThreadSuspend(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userThread = regs->arg0;
    uint64_t userSuspend = regs->arg1;

    if (userSuspend != 0 && userSuspend != 1) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysThreadSuspend(&invoke, userThread, userSuspend != 0)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}
