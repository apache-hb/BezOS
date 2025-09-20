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

    SysLog.dbgf("VmemCreate: BaseAddress: ", createInfo.BaseAddress, ", Size: ", km::Hex(createInfo.Size),
                  ", Alignment: ", km::Hex(createInfo.Alignment), ", Access: ", km::Hex(createInfo.Access),
                  ", Process: ", km::Hex(createInfo.Process));

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    void *vaddr = nullptr;
    if (OsStatus status = sys::SysVmemCreate(&invoke, createInfo, &vaddr)) {
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
        SysLog.dbgf("VmemMap: Failed to read map info: ", OsStatusId(status));
        return km::CallError(status);
    }

    SysLog.dbgf("VmemMap: SrcAddress: ", km::Hex(mapInfo.SrcAddress), ", DstAddress: ", km::Hex(mapInfo.DstAddress),
                  ", Size: ", km::Hex(mapInfo.Size), ", Access: ", km::Hex(mapInfo.Access), ", Source: ",
                  km::Hex(mapInfo.Source), ", Process: ", km::Hex(mapInfo.Process));

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    void *vaddr = nullptr;
    if (OsStatus status = sys::SysVmemMap(&invoke, mapInfo, &vaddr)) {
        return km::CallError(status);
    }

    return km::CallOk(vaddr);
}

OsCallResult um::VmemDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewVmemDestroy(system, context, regs);
}
