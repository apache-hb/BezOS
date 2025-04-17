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

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysThreadCreate(&invoke, createInfo, &thread)) {
        return km::CallError(status);
    }

    return km::CallOk(thread);
}

static OsCallResult NewThreadDestroy(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userThread = regs->arg0;

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysThreadDestroy(&invoke, eOsThreadFinished, userThread)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    sys2::YieldCurrentThread();
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
