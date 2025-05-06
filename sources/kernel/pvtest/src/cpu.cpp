#include "pvtest/cpu.hpp"
#include "pvtest/pvtest.hpp"
#include "pvtest/machine.hpp"

#include "memory/memory.hpp"

#include "arch/cr3.hpp"

#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include <utility>

#include "common/util/defer.hpp"

static pv::CpuCore *fCpuCore = nullptr;

static constexpr int kSigDestroy = SIGUSR1;
static constexpr int kSigLaunch = SIGUSR2;

static void SignalWriteV(int fd, const char *fmt, va_list args) {
    // TODO: vsnprintf is technically not signal safe, but i dont think glibc does anything not signal-unsafe in it...
    char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    write(fd, buffer, len);
}

[[gnu::format(printf, 2, 3)]]
static void SignalWrite(int fd, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SignalWriteV(fd, fmt, args);
    va_end(args);
}

[[noreturn, gnu::format(printf, 1, 2)]]
static void SignalAssert(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SignalWriteV(STDERR_FILENO, fmt, args);
    va_end(args);
    pv::ExitFork(EXIT_FAILURE);
}

void pv::CpuCore::setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept {
    switch (reg) {
    case X86_REG_RAX: mcontext->gregs[REG_RAX] = value; break;
    case X86_REG_RBX: mcontext->gregs[REG_RBX] = value; break;
    case X86_REG_RCX: mcontext->gregs[REG_RCX] = value; break;
    case X86_REG_RDX: mcontext->gregs[REG_RDX] = value; break;
    case X86_REG_RSI: mcontext->gregs[REG_RSI] = value; break;
    case X86_REG_RDI: mcontext->gregs[REG_RDI] = value; break;
    case X86_REG_RBP: mcontext->gregs[REG_RBP] = value; break;
    case X86_REG_RSP: mcontext->gregs[REG_RSP] = value; break;
    case X86_REG_R8:  mcontext->gregs[REG_R8]  = value; break;
    case X86_REG_R9:  mcontext->gregs[REG_R9]  = value; break;
    case X86_REG_R10: mcontext->gregs[REG_R10] = value; break;
    case X86_REG_R11: mcontext->gregs[REG_R11] = value; break;
    case X86_REG_R12: mcontext->gregs[REG_R12] = value; break;
    case X86_REG_R13: mcontext->gregs[REG_R13] = value; break;
    case X86_REG_R14: mcontext->gregs[REG_R14] = value; break;
    case X86_REG_R15: mcontext->gregs[REG_R15] = value; break;
    default:
        SignalAssert("pv::CpuCore::setRegisterOperand() unknown register %d\n", std::to_underlying(reg));
    }
}

uint64_t pv::CpuCore::getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept {
    switch (reg) {
    case X86_REG_RAX: return mcontext->gregs[REG_RAX];
    case X86_REG_RBX: return mcontext->gregs[REG_RBX];
    case X86_REG_RCX: return mcontext->gregs[REG_RCX];
    case X86_REG_RDX: return mcontext->gregs[REG_RDX];
    case X86_REG_RSI: return mcontext->gregs[REG_RSI];
    case X86_REG_RDI: return mcontext->gregs[REG_RDI];
    case X86_REG_RBP: return mcontext->gregs[REG_RBP];
    case X86_REG_RSP: return mcontext->gregs[REG_RSP];
    case X86_REG_R8:  return mcontext->gregs[REG_R8];
    case X86_REG_R9:  return mcontext->gregs[REG_R9];
    case X86_REG_R10: return mcontext->gregs[REG_R10];
    case X86_REG_R11: return mcontext->gregs[REG_R11];
    case X86_REG_R12: return mcontext->gregs[REG_R12];
    case X86_REG_R13: return mcontext->gregs[REG_R13];
    case X86_REG_R14: return mcontext->gregs[REG_R14];
    case X86_REG_R15: return mcontext->gregs[REG_R15];
    case X86_REG_RIP: return mcontext->gregs[REG_RIP];
    default:
        SignalAssert("pv::CpuCore::getRegisterOperand() unknown register %d\n", std::to_underlying(reg));
    }
}

uint64_t pv::CpuCore::getMemoryOperand(mcontext_t *mcontext, const x86_op_mem *op) noexcept {
    uint64_t base = (op->base == X86_REG_INVALID) ? 0 : getRegisterOperand(mcontext, op->base);
    uint64_t index = (op->index == X86_REG_INVALID) ? 0 : getRegisterOperand(mcontext, op->index);
    return base + (index * op->scale) + op->disp;
}

OsStatus pv::CpuCore::resolveVirtualAddress(sm::VirtualAddress address, sm::PhysicalAddress *result) noexcept {
    if (!mMemory->getGuestRange().contains(address)) {
        auto range = mMemory->getGuestRange();
        SignalAssert("virtual address %p escapes guest memory %p-%p\n",
                     (void*)address, (void*)range.front, (void*)range.back);
    }
    auto [pml4e, pdpte, pdte, pte] = km::GetAddressParts(address.address);

    sm::VirtualAddress l4Address = mMemory->getHostAddress(x64::Cr3::of(cr3).address());
    x64::PageMapLevel4 *l4 = std::bit_cast<x64::PageMapLevel4*>(l4Address);
    x64::pml4e &t4 = l4->entries[pml4e];
    if (!t4.present()) {
        SignalWrite(STDERR_FILENO, "PML4E %d not present\n", pml4e);
        return OsStatusInvalidAddress;
    }

    sm::VirtualAddress l3Address = mMemory->getHostAddress(mPageBuilder->address(t4));
    x64::PageMapLevel3 *l3 = std::bit_cast<x64::PageMapLevel3*>(l3Address);
    x64::pdpte &t3 = l3->entries[pdpte];
    if (!t3.present()) {
        SignalWrite(STDERR_FILENO, "PDPTE %d not present (host: %p ram: %zx)\n", pdpte, (void*)l3Address, mPageBuilder->address(t4));
        return OsStatusInvalidAddress;
    }

    if (t3.is1g()) {
        uintptr_t pdpteOffset = address.address & 0x3FF'000;
        *result = sm::PhysicalAddress { mPageBuilder->address(t3) + pdpteOffset };
        return OsStatusSuccess;
    }

    sm::VirtualAddress l2Address = mMemory->getHostAddress(mPageBuilder->address(t3));
    x64::PageMapLevel2 *l2 = std::bit_cast<x64::PageMapLevel2*>(l2Address);
    x64::pdte &t2 = l2->entries[pdte];
    if (!t2.present()) {
        SignalWrite(STDERR_FILENO, "PDTE %d not present (host: %p ram: %zx)\n", pdte, (void*)l2Address, mPageBuilder->address(t3));
        return OsStatusInvalidAddress;
    }

    if (t2.is2m()) {
        uintptr_t pdteOffset = address.address & 0x1FF'000;
        *result = sm::PhysicalAddress { mPageBuilder->address(t2) + pdteOffset };
        return OsStatusSuccess;
    }

    sm::VirtualAddress ptAddress = mMemory->getHostAddress(mPageBuilder->address(t2));
    x64::PageTable *pt = std::bit_cast<x64::PageTable*>(ptAddress);
    x64::pte &t1 = pt->entries[pte];
    if (!t1.present()) {
        SignalWrite(STDERR_FILENO, "PTE %d not present (host: %p ram: %zx)\n", pte, (void*)ptAddress, mPageBuilder->address(t2));
        return OsStatusInvalidAddress;
    }

    *result = sm::PhysicalAddress { mPageBuilder->address(t1) };
    return OsStatusSuccess;
}

void pv::CpuCore::emulate_cli(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_sti(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_lidt(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_lgdt(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_ltr(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_in(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_out(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_invlpg(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_swapgs(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_rdmsr(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_wrmsr(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_hlt(mcontext_t *, cs_insn *) { }
void pv::CpuCore::emulate_iretq(mcontext_t *, cs_insn *) { }

void pv::CpuCore::emulate_mmu(mcontext_t *mcontext, cs_insn *insn) {
    cs_detail *detail = insn->detail;
    cs_x86 x86 = detail->x86;
    for (uint8_t i = 0; i < x86.op_count; i++) {
        cs_x86_op *op = &x86.operands[i];
        if (op->type == X86_OP_MEM) {
            uint64_t value = getMemoryOperand(mcontext, &op->mem);
            sm::PhysicalAddress real;
            OsStatus status = resolveVirtualAddress(value, &real);
            if (status != OsStatusSuccess) {
                SignalAssert("resolveVirtualAddress %p failed: %d\n", (void*)value, int(status));
                return;
            }

            real = sm::rounddown(real.address, x64::kPageSize);
            value = sm::rounddown(value, x64::kPageSize);

            mMemory->mapGuestPage(value, km::PageFlags::eAll, real);
        }
    }
}

void pv::CpuCore::sigsegv(mcontext_t *mcontext) {
    uintptr_t rip = mcontext->gregs[REG_RIP];
    size_t size = 15;
    uint64_t pc = rip;
    const uint8_t *ptr = (const uint8_t *)pc;
    if (!cs_disasm_iter(mCapstone, &ptr, &size, &pc, mInstruction)) {
        cs_err cserr = cs_errno(mCapstone);
        PV_POSIX_ERROR(EINVAL, "cs_disasm_iter failed: %s (%d)\n", cs_strerror(cserr), cserr);
    }

    SignalWrite(STDERR_FILENO, "%p: %s %s\n", (void*)rip, mInstruction->mnemonic, mInstruction->op_str);

    switch (mInstruction->id) {
    case X86_INS_RDMSR:
        emulate_rdmsr(mcontext, mInstruction);
        break;
    case X86_INS_WRMSR:
        emulate_wrmsr(mcontext, mInstruction);
        break;
    case X86_INS_IN:
        emulate_in(mcontext, mInstruction);
        break;
    case X86_INS_OUT:
        emulate_out(mcontext, mInstruction);
        break;
    case X86_INS_LGDT:
        emulate_lgdt(mcontext, mInstruction);
        break;
    case X86_INS_LIDT:
        emulate_lidt(mcontext, mInstruction);
        break;
    case X86_INS_LTR:
        emulate_ltr(mcontext, mInstruction);
        break;
    case X86_INS_INVLPG:
        emulate_invlpg(mcontext, mInstruction);
        break;
    case X86_INS_SWAPGS:
        emulate_swapgs(mcontext, mInstruction);
        break;
    case X86_INS_HLT:
        emulate_hlt(mcontext, mInstruction);
        break;
    case X86_INS_IRETQ:
        emulate_iretq(mcontext, mInstruction);
        break;
    case X86_INS_CLI:
        emulate_cli(mcontext, mInstruction);
        break;
    case X86_INS_STI:
        emulate_sti(mcontext, mInstruction);
        break;

    default:
        // If the sigsegv is not due to a privileged instruction, its probably a page fault
        emulate_mmu(mcontext, mInstruction);
        break;
    }
}

void pv::CpuCore::run() {
    pv::Machine::initChild();

    size_t sigstksz = SIGSTKSZ;
    mSigStack = stack_t {
        .ss_sp = malloc(sigstksz),
        .ss_flags = 0,
        .ss_size = sigstksz,
    };

    defer { free(mSigStack.ss_sp); };

    if (cs_err err = cs_open(CS_ARCH_X86, CS_MODE_64, &mCapstone)) {
        PV_POSIX_ERROR(err, "cs_open: %s (%d)\n", cs_strerror(err), err);
    }

    cs_option(mCapstone, CS_OPT_DETAIL, CS_OPT_ON);

    defer { cs_close(&mCapstone); };

    mInstruction = cs_malloc(mCapstone);
    if (!mInstruction) {
        PV_POSIX_ERROR(ENOMEM, "cs_malloc: %s\n", strerror(ENOMEM));
    }
    defer { cs_free(mInstruction, 1); };

    installSignals();

    if (setjmp(mDestroyTarget)) {
        return;
    }

    if (setjmp(mLaunchTarget)) {
        PvTestContextSwitch(&mLaunchContext.gregs);
        return;
    }

    mReady.flag.test_and_set();

    sigset_t sigset;
    PV_POSIX_CHECK(sigemptyset(&sigset));
    PV_POSIX_CHECK(sigaddset(&sigset, kSigDestroy));
    PV_POSIX_CHECK(sigaddset(&sigset, kSigLaunch));

    siginfo_t siginfo;
    PV_POSIX_CHECK(sigwaitinfo(&sigset, &siginfo));
    switch (siginfo.si_signo) {
    case kSigDestroy:
        break;
    case kSigLaunch:
        PvTestContextSwitch(&mLaunchContext.gregs);
        break;

    default:
        SignalAssert("pv::CpuCore::start() unknown signal %d\n", siginfo.si_signo);
    }
}

int pv::CpuCore::start(void *arg) {
    CpuCore *self = reinterpret_cast<CpuCore *>(arg);
    fCpuCore = self;

    self->run();
    self->mChild = 0;

    pv::Machine::finalizeChild();

    return 0;
}

void pv::CpuCore::destroy(mcontext_t *) {
    longjmp(mDestroyTarget, 1);
}

void pv::CpuCore::launch(mcontext_t *mcontext) {
    mLaunchContext = *mcontext;
    longjmp(mLaunchTarget, 1);
}

void pv::CpuCore::sendInitMessage(mcontext_t context) {
    mLaunchContext = context;
    sigval val {
        .sival_ptr = this,
    };

    while (!mReady.flag.test_and_set()) {
        sched_yield();
    }

    PV_POSIX_CHECK(sigqueue(mChild, kSigLaunch, val));
}

pv::CpuCore::CpuCore(Memory *memory, const km::PageBuilder *pager)
    : mChildStack(new std::byte[0x1000 * 32]())
    , mPageBuilder(pager)
    , mMemory(memory)
{
    int flags = CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD;
    PV_POSIX_CHECK((mChild = clone(&CpuCore::start, std::bit_cast<void*>(mChildStack.get() + 0x1000 * 32), flags, this)));
}

pv::CpuCore::~CpuCore() {
    if (mChild == 0) {
        return;
    }

    while (!mReady.flag.test_and_set()) {
        sched_yield();
    }

    sigval val {
        .sival_ptr = this,
    };

    PV_POSIX_CHECK(sigqueue(mChild, kSigDestroy, val));

    int status = 0;
    PV_POSIX_CHECK(waitpid(mChild, &status, 0));
}

void pv::CpuCore::installSignals() {
    sigset_t sigset;
    PV_POSIX_CHECK(sigprocmask(SIG_BLOCK, nullptr, &sigset));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGSEGV));
    PV_POSIX_CHECK(sigdelset(&sigset, kSigLaunch));
    PV_POSIX_CHECK(sigdelset(&sigset, kSigDestroy));
    PV_POSIX_CHECK(sigprocmask(SIG_SETMASK, &sigset, nullptr));

    PV_POSIX_CHECK(sigaltstack(&mSigStack, nullptr));

    struct sigaction sigsegv {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            fCpuCore->sigsegv(mcontext);
            SignalWrite(STDERR_FILENO, "sigsegv handled: %p\n", (void*)mcontext->gregs[REG_RIP]);
        },
        .sa_flags = SA_SIGINFO | SA_ONSTACK,
    };

    struct sigaction sigdestroy {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            fCpuCore->destroy(mcontext);
        },
        .sa_flags = SA_SIGINFO | SA_ONSTACK,
    };

    struct sigaction siglaunch {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            fCpuCore->launch(mcontext);
        },
        .sa_flags = SA_SIGINFO | SA_ONSTACK,
    };

    PV_POSIX_CHECK(sigaction(SIGSEGV, &sigsegv, nullptr));
    PV_POSIX_CHECK(sigaction(kSigDestroy, &sigdestroy, nullptr));
    PV_POSIX_CHECK(sigaction(kSigLaunch, &siglaunch, nullptr));
}
