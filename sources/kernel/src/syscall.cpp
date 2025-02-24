#include "syscall.hpp"

#include "gdt.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "thread.hpp"

static constexpr x64::ModelRegister<0xC0000081, x64::RegisterAccess::eReadWrite> kStar;
static constexpr x64::ModelRegister<0xC0000082, x64::RegisterAccess::eReadWrite> kLStar;
static constexpr x64::ModelRegister<0xC0000084, x64::RegisterAccess::eReadWrite> kFMask;

static constexpr size_t kStackSize = 0x4000 * 4;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsSystemCallStack;

static constinit km::SystemCallHandler gSystemCalls[256] = { nullptr };

void km::AddSystemCall(uint8_t function, SystemCallHandler handler) {
    gSystemCalls[function] = handler;
}

extern "C" void KmSystemEntry(void);

extern "C" uint64_t KmSystemCallStackTlsOffset;

extern "C" OsCallResult KmSystemDispatchRoutine(km::SystemCallContext *context) {
    if (__gsbase() == 0) {
        KmDebugMessageUnlocked("[SYS] System call stack not setup.\n");
        KmHalt();
    }

    volatile uint8_t function = context->function;
    if (km::SystemCallHandler handler = gSystemCalls[function]) {
        OsCallResult result = handler(context->arg0, context->arg1, context->arg2, context->arg3);
        KM_CHECK(context->function == function, "System call handler changed function number.");
        return result;
    }

    KmDebugMessage("[SYS] Unknown function ", km::Hex(context->function), "\n");
    return OsCallResult { .Status = OsStatusInvalidFunction, .Value = 0 };
}

void km::SetupUserMode() {
    tlsSystemCallStack = aligned_alloc(kStackSize, x64::kPageSize);

    void *rsp0 = aligned_alloc(kStackSize, x64::kPageSize);
    tlsTaskState->rsp0 = (uintptr_t)rsp0 + kStackSize;

    KmSystemCallStackTlsOffset = tlsSystemCallStack.tlsOffset();

    uint64_t star = 0;

    // Store the kernel mode code segment and stack segment in the STAR MSR
    // The stack selector is chosen as the next entry in the gdt
    star |= uint64_t(SystemGdt::eLongModeCode * 0x8) << 32;

    // And the user mode code segment and stack segment
    star |= uint64_t(((SystemGdt::eLongModeUserCode * 0x8) - 0x10) | 0b11) << 48;

    // Enable syscall/sysret
    uint64_t efer = kEfer.load();
    kEfer.store(efer | (1 << 0));

    // Mask the interrupt flag in RFLAGS
    uint64_t fmask = (1 << 9);

    kFMask.store(fmask);

    // Store the syscall entry point in the LSTAR MSR
    kLStar.store((uintptr_t)KmSystemEntry);

    kStar.store(star);
}

void km::EnterUserMode(km::IsrContext state) {
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
