#include "system/create.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include <bezos/facility/vmem.h>

#include "syscall.hpp"

static OsCallResult NewVmemCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsVmemCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    void *vaddr = nullptr;
    if (OsStatus status = sys2::SysVmemCreate(&invoke, createInfo, &vaddr)) {
        return km::CallError(status);
    }

    return km::CallOk(vaddr);
}

static OsCallResult NewVmemDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::VmemCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewVmemCreate(system, context, regs);
}

OsCallResult um::VmemMap(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userMapInfo = regs->arg0;

    OsVmemMapInfo mapInfo{};

    if (OsStatus status = context->readObject(userMapInfo, &mapInfo)) {
        return km::CallError(status);
    }

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    void *vaddr = nullptr;
    if (OsStatus status = sys2::SysVmemMap(&invoke, mapInfo, &vaddr)) {
        return km::CallError(status);
    }

    return km::CallOk(vaddr);
}

OsCallResult um::VmemDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewVmemDestroy(system, context, regs);
}
