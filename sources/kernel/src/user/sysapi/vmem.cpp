#include "user/sysapi.hpp"

#include <bezos/facility/vmem.h>

#include "syscall.hpp"

OsCallResult um::VmemCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
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

OsCallResult um::VmemDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}
