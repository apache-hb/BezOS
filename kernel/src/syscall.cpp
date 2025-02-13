#include "syscall.hpp"

#include "gdt.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "thread.hpp"

static constexpr x64::ModelRegister<0xC0000081, x64::RegisterAccess::eReadWrite> kStar;
static constexpr x64::ModelRegister<0xC0000082, x64::RegisterAccess::eReadWrite> kLStar;
static constexpr x64::ModelRegister<0xC0000084, x64::RegisterAccess::eReadWrite> kFMask;

static constexpr size_t kStackSize = 0x1000;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsSystemCallStack;

static constinit km::SystemCallHandler gSystemCalls[256] = { nullptr };

void km::AddSystemCall(uint8_t function, SystemCallHandler handler) {
    gSystemCalls[function] = handler;
}

extern "C" void KmSystemEntry(void);

extern "C" uint64_t KmSystemCallStackTlsOffset;

extern "C" OsCallResult KmSystemDispatchRoutine(km::SystemCallContext *context) {
    KmDebugMessage("[SYSCALL] Function: ", km::Hex(context->function), ", Arg0: ", km::Hex(context->arg0), ", Arg1: ", km::Hex(context->arg1), ", Arg2: ", km::Hex(context->arg2), ", Arg3: ", km::Hex(context->arg3), "\n");

    if (km::SystemCallHandler handler = gSystemCalls[uint8_t(context->function)]) {
        return handler(context->arg0, context->arg1, context->arg2, context->arg3);
    }

    return OsCallResult { .Status = OsStatusInvalidFunction, .Value = 0 };
}

void km::SetupUserMode(mem::IAllocator *allocator) {
    tlsSystemCallStack = allocator->allocateAligned(kStackSize, x64::kPageSize);

    void *rsp0 = allocator->allocateAligned(kStackSize, x64::kPageSize);
    tlsTaskState->rsp0 = (uintptr_t)rsp0 + kStackSize;

    KmSystemCallStackTlsOffset = tlsSystemCallStack.tlsOffset();

    // Enable syscall/sysret
    uint64_t efer = kEfer.load();
    kEfer.store(efer | (1 << 0));

    kFMask.store(0xFFFF'FFFF);

    // Store the syscall entry point in the LSTAR MSR
    kLStar.store((uint64_t)KmSystemEntry);

    uint64_t star = 0;

    // Store the kernel mode code segment and stack segment in the STAR MSR
    // The stack selector is chosen as the next entry in the gdt
    star |= uint64_t(SystemGdt::eLongModeCode * 0x8) << 32;

    // And the user mode code segment and stack segment
    star |= uint64_t(((SystemGdt::eLongModeUserCode * 0x8) - 0x10) | 0b11) << 48;

    kStar.store(star);
}

void km::EnterUserMode(x64::RegisterState state) {
    asm volatile (
        "movq %[mrsp], %%rsp\n"
        "movq %[mrbp], %%rbp\n"
        "movq %[mrip], %%rcx\n"
        "movq %[meflags], %%r11\n"
        "movq %[mrdi], %%rdi\n"
        "movq %[mrsi], %%rsi\n"
        "movq %[mrdx], %%rdx\n"
        "\n"
        "swapgs\n"
        "sysretq\n"
        : [mrsp] "+m" (state.rsp), [mrbp] "+m" (state.rbp)
        : [mrip] "m" (state.rip), [meflags] "m" (state.rflags),
          [mrdi] "m" (state.rdi), [mrsi] "m" (state.rsi),
          [mrdx] "m" (state.rdx)
        : "memory", "cc"
    );
}
