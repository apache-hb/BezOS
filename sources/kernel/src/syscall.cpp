#include "syscall.hpp"

#include "gdt.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "process/schedule.hpp"
#include "thread.hpp"
#include "user/user.hpp"

static constexpr x64::ModelRegister<0xC0000081, x64::RegisterAccess::eReadWrite> kStar;
static constexpr x64::ModelRegister<0xC0000082, x64::RegisterAccess::eReadWrite> kLStar;
static constexpr x64::ModelRegister<0xC0000084, x64::RegisterAccess::eReadWrite> kFMask;

static constexpr size_t kStackSize = 0x4000 * 4;

static constinit km::BetterCallHandler gSystemCalls[256] = { nullptr };

void km::AddSystemCall(uint8_t function, BetterCallHandler handler) {
    gSystemCalls[function] = handler;
}

extern "C" void KmSystemEntry(void);

extern "C" OsCallResult KmSystemDispatchRoutine(km::SystemCallRegisterSet *regs) {
    uint8_t function = regs->function;
    if (km::BetterCallHandler handler = gSystemCalls[function]) {
        km::CallContext context{};
        return handler(&context, regs);
    }

    KmDebugMessage("[SYS] Unknown function ", km::Hex(regs->function), "\n");
    return OsCallResult { .Status = OsStatusInvalidFunction, .Value = 0 };
}

void km::SetupUserMode(SystemMemory *memory) {
    void *rsp0 = (void*)memory->allocateStack(kStackSize).vaddr;
    tlsTaskState->rsp0 = (uintptr_t)rsp0 + kStackSize;

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

km::Process *km::CallContext::process() {
    return km::GetCurrentProcess();
}

km::Thread *km::CallContext::thread() {
    return km::GetCurrentThread();
}

km::PageTables& km::CallContext::ptes() {
    return km::GetProcessPageTables();
}

bool km::CallContext::isMapped(uint64_t front, uint64_t back, PageFlags flags) {
    return km::IsRangeMapped(ptes(), (void*)front, (void*)back, flags);
}

OsStatus km::CallContext::readMemory(uint64_t address, size_t size, void *dst) {
    return km::CopyUserMemory(ptes(), address, size, dst);
}

OsStatus km::CallContext::writeMemory(uint64_t address, const void *src, size_t size) {
    return km::WriteUserMemory(ptes(), (void*)address, src, size);
}

OsStatus km::CallContext::readString(uint64_t front, uint64_t back, size_t limit, stdx::String *dst) {
    return readArray(front, back, limit, dst);
}

OsStatus km::CallContext::readRange(uint64_t front, uint64_t back, void *dst, size_t size) {
    return km::ReadUserMemory(ptes(), (void*)front, (void*)back, dst, size);
}

OsStatus km::CallContext::writeRange(uint64_t address, const void *front, const void *back) {
    return km::WriteUserMemory(ptes(), (void*)address, front, (uintptr_t)back - (uintptr_t)front);
}
