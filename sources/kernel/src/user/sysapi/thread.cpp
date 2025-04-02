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
    stdx::String name;
    km::Process *process = nullptr;

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
        return km::CallError(status);
    }

    if (OsStatus status = um::SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    stdx::String stackName = name + " STACK";
    km::Thread *thread = system->objects->createThread(std::move(name), process);

    OsMachineContext cpuState = createInfo.CpuState;

    thread->state = km::IsrContext {
        .rax = cpuState.rax,
        .rbx = cpuState.rbx,
        .rcx = cpuState.rcx,
        .rdx = cpuState.rdx,
        .rdi = cpuState.rdi,
        .rsi = cpuState.rsi,
        .r8 = cpuState.r8,
        .r9 = cpuState.r9,
        .r10 = cpuState.r10,
        .r11 = cpuState.r11,
        .r12 = cpuState.r12,
        .r13 = cpuState.r13,
        .r14 = cpuState.r14,
        .r15 = cpuState.r15,

        .rbp = cpuState.rbp,
        .rip = cpuState.rip,
        .cs = (km::SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .rsp = cpuState.rsp,
        .ss = (km::SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };
    thread->tlsAddress = cpuState.fs;

    return km::CallOk(thread->publicId());
}

OsCallResult um::ThreadDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::ThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    km::YieldCurrentThread();
    return km::CallOk(0zu);
}
