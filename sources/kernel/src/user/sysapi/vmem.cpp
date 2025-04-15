#include "system/create.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include <bezos/facility/vmem.h>

#include "syscall.hpp"

static OsCallResult UserVmemCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsVmemCreateInfo createInfo{};
    km::Process *process = nullptr;
    km::AddressMapping mapping{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if ((createInfo.Size % x64::kPageSize != 0) || (createInfo.Size < x64::kPageSize)) {
        return km::CallError(OsStatusInvalidInput);
    }

    if (OsStatus status = um::SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    OsStatus status = AllocateMemory(system->memory->pmmAllocator(), *process->ptes.get(), createInfo.Size / x64::kPageSize, &mapping);
    if (status != OsStatusSuccess) {
        return km::CallError(status);
    }

    memset((void*)mapping.vaddr, 0, createInfo.Size);

    KmDebugMessage("[VMEM] Created mapping: ", mapping, "\n");

    return km::CallOk(mapping.vaddr);
}

static OsCallResult UserVmemDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

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
    if constexpr (kUseNewSystem) {
        return NewVmemCreate(system, context, regs);
    } else {
        return UserVmemCreate(system, context, regs);
    }
}

OsCallResult um::VmemMap(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::VmemDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewVmemDestroy(system, context, regs);
    } else {
        return UserVmemDestroy(system, context, regs);
    }
}
