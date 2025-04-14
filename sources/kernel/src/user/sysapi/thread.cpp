#include "gdt.hpp"
#include "system/create.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "process/system.hpp"
#include "process/schedule.hpp"
#include "process/thread.hpp"

#include "syscall.hpp"

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

static OsCallResult UserThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
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

static OsCallResult UserThreadDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

static OsCallResult UserThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    km::YieldCurrentThread();
    return km::CallOk(0zu);
}

static OsCallResult NewThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsThreadCreateInfo createInfo{};
    OsThreadHandle thread = OS_HANDLE_INVALID;

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysCreateThread(&invoke, createInfo, &thread)) {
        return km::CallError(status);
    }

    return km::CallOk(thread);
}

static OsCallResult NewThreadDestroy(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userThread = regs->arg0;

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysDestroyThread(&invoke, eOsThreadFinished, userThread)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewThreadSleep(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    km::YieldCurrentThread();
    return km::CallOk(0zu);
}

OsCallResult um::ThreadCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewThreadCreate(system, context, regs);
    } else {
        return UserThreadCreate(system, context, regs);
    }
}

OsCallResult um::ThreadDestroy(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewThreadDestroy(system, context, regs);
    } else {
        return UserThreadDestroy(system, context, regs);
    }
}

OsCallResult um::ThreadSleep(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewThreadSleep(system, context, regs);
    } else {
        return UserThreadSleep(system, context, regs);
    }
}
