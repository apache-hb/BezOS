#include "gdt.hpp"
#include "user/sysapi.hpp"

#include "process/system.hpp"
#include "process/schedule.hpp"
#include "process/thread.hpp"

#include "syscall.hpp"

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

OsCallResult um::ThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsThreadCreateInfo createInfo{};
    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if ((createInfo.StackSize % x64::kPageSize != 0) || (createInfo.StackSize < x64::kPageSize)) {
        return km::CallError(OsStatusInvalidInput);
    }

    stdx::String name;
    if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
        return km::CallError(status);
    }

    km::Process *process = nullptr;

    if (OsStatus status = um::SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    stdx::String stackName = name + " STACK";
    km::Thread *thread = system->objects->createThread(std::move(name), process);

    km::AddressMapping mapping{};
    OsStatus status = AllocateMemory(system->memory->pmmAllocator(), *process->ptes.get(), createInfo.StackSize / x64::kPageSize, &mapping);
    if (status != OsStatusSuccess) {
        return km::CallError(status);
    }

    thread->userStack = mapping;
    thread->state = km::IsrContext {
        .rbp = (uintptr_t)((uintptr_t)mapping.vaddr + createInfo.StackSize),
        .rip = (uintptr_t)createInfo.EntryPoint,
        .cs = (km::SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .rsp = (uintptr_t)((uintptr_t)mapping.vaddr + createInfo.StackSize),
        .ss = (km::SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };
    return km::CallOk(thread->publicId());
}

OsCallResult um::ThreadDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::ThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    km::YieldCurrentThread();
    return km::CallOk(0zu);
}
