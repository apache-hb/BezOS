#include "pvtest/machine.hpp"
#include "pvtest/system.hpp"
#include "pvtest/pvtest.hpp"
#include "setup.hpp"

#include <atomic>
#include <format>
#include <linux/prctl.h>
#include <sys/mman.h>
#include <rpmalloc.h>
#include <sys/prctl.h>
#include <signal.h>

static int gSharedMemoryFd = -1;
static sm::VirtualAddress gSharedHostMemory = nullptr;
static std::atomic<uintptr_t> gSharedHostBase;
static constexpr size_t kSharedMemorySize = sm::gigabytes(2).bytes();
static pv::Machine *gMachine = nullptr;

static void *RpMallocSharedMap(size_t size, size_t align, size_t *offset, size_t *mappedSize) {
    size_t mapSize = size + align;
    sm::VirtualAddress result = nullptr;
    size_t resultSize = 0;
    size_t resultOffset = 0;

    if (mapSize > kSharedMemorySize) {
        fprintf(stderr, "rpmalloc: too large %zu\n", size);
        return nullptr;
    }

    result = gSharedHostBase.fetch_add(mapSize);
    if (result + mapSize > (gSharedHostMemory + kSharedMemorySize)) {
        fprintf(stderr, "rpmalloc: out of memory %p %p 0x%zx\n", (void*)gSharedHostMemory, (void*)result, mapSize);
        return nullptr;
    }

    if (align) {
        size_t padding = (result.address & (uintptr_t)(align - 1));
        if (padding)
            padding = align - padding;
        result += padding;
        resultOffset = padding;
    }

    *offset = resultOffset;
    *mappedSize = resultSize;
    return result;
}

static void RpMallocSharedUnmap(void *ptr, size_t offset, size_t size) {
    fprintf(stderr, "rpmalloc: unmap %p %zu %zu\n", ptr, offset, size);
}

static void RpMallocError(const char *msg) {
    fprintf(stderr, "rpmalloc: error %s\n", msg);
}

static void RpMallocCommit(void *ptr, size_t size) {
    fprintf(stderr, "rpmalloc: commit %p %zu\n", ptr, size);
}

static void RpMallocDecommit(void *ptr, size_t size) {
    fprintf(stderr, "rpmalloc: decommit %p %zu\n", ptr, size);
}

static int RpMallocMapFailCallback(size_t size) {
    fprintf(stderr, "rpmalloc: map fail %zu\n", size);
    return false;
}

static rpmalloc_interface_t gMallocInterface {
    .memory_map = RpMallocSharedMap,
    .memory_commit = RpMallocCommit,
    .memory_decommit = RpMallocDecommit,
    .memory_unmap = RpMallocSharedUnmap,
    .map_fail_callback = RpMallocMapFailCallback,
    .error_callback = RpMallocError,
};

void pv::setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept {
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

uint64_t pv::getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept {
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

uint64_t pv::getOperand(mcontext_t *mcontext, cs_x86_op operand) noexcept {
    if (operand.type == X86_OP_REG) {
        return getRegisterOperand(mcontext, operand.reg);
    } else if (operand.type == X86_OP_IMM) {
        return operand.imm;
    } else {
        SignalAssert("pv::getOperand() unknown operand type %d\n", std::to_underlying(operand.type));
    }
}

pv::Machine::Machine(size_t cores, off64_t memorySize)
    : mPageBuilder(48, 48, km::GetDefaultPatLayout())
    , mMemory(memorySize)
{
    mCores.reserve(cores + 1);
    for (size_t i = 0; i < cores; i++) {
        mCores.emplace_back(getMemory(), getPageBuilder(), i);
    }
}

pv::Machine::~Machine() {
    if (mSigStack.ss_sp) {
        free(mSigStack.ss_sp);
    }

    if (mInstruction) {
        cs_free(mInstruction, 1);
    }

    if (mCapstone) {
        cs_close(&mCapstone);
    }
}

void pv::Machine::bspInit(mcontext_t mcontext) {
    mCores[0].sendInitMessage(mcontext);
}

void pv::Machine::emulate_cli(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_sti(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_lidt(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_lgdt(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_ltr(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_in(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_out(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_invlpg(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_swapgs(mcontext_t *mcontext, cs_insn *insn) { }

void pv::Machine::emulate_rdmsr(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;

    uint64_t value = rdmsr(msr);
    mcontext->gregs[REG_RAX] = value & 0xFFFFFFFF;
    mcontext->gregs[REG_RDX] = (value >> 32) & 0xFFFFFFFF;
    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::emulate_wrmsr(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;
    uint32_t low = mcontext->gregs[REG_RAX] & 0xFFFFFFFF;
    uint32_t high = mcontext->gregs[REG_RDX] & 0xFFFFFFFF;

    uint64_t value = low | ((uint64_t)high << 32);
    wrmsr(msr, value);

    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::emulate_hlt(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_iretq(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_mmu(mcontext_t *mcontext, cs_insn *insn) { }

void pv::Machine::emulateReadControl(mcontext_t *mcontext, cs_insn *insn, x86_reg reg, uint64_t value) {

}

void pv::Machine::emulateWriteControl(mcontext_t *mcontext, cs_insn *insn, x86_reg reg, uint64_t value) {

}

static bool isControlRegisterOperand(cs_x86_op *op) {
    return op->type == X86_OP_REG && (op->reg >= X86_REG_CR0 && op->reg <= X86_REG_CR15);
}

void pv::Machine::emulate_mov(mcontext_t *mcontext, cs_insn *insn) {
    if (mcontext->gregs[REG_CR2] != 0) {
        emulate_mmu(mcontext, insn);
        return;
    }

    cs_detail *detail = insn->detail;
    cs_x86 x86 = detail->x86;
    // mov with a control register as its first operand is a write
    if (isControlRegisterOperand(&x86.operands[0])) {
        cs_x86_op *op = &x86.operands[0];
        if (op->access != CS_AC_WRITE) {
            SignalAssert("pv::Machine::emulate_mov() control register %s is not a write\n", cs_reg_name(mCapstone, op->reg));
        }
        emulateWriteControl(mcontext, insn, op->reg, getOperand(mcontext, x86.operands[1]));
    } else if (isControlRegisterOperand(&x86.operands[1])) {
        // mov with a control register as its second operand is a read
        cs_x86_op *op = &x86.operands[1];
        if (op->access != CS_AC_READ) {
            SignalAssert("pv::Machine::emulate_mov() control register %s is not a read\n", cs_reg_name(mCapstone, op->reg));
        }
        emulateReadControl(mcontext, insn, op->reg, getOperand(mcontext, x86.operands[0]));
    }

    SignalAssert("pv::Machine::emulate_mov() unhandled instruction %p %s %s\n", (void*)mcontext->gregs[REG_RIP], insn->mnemonic, insn->op_str);
}

void pv::Machine::emulate_xgetbv(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t reg = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;
    uint64_t value = xgetbv(reg);
    mcontext->gregs[REG_RAX] = value & 0xFFFFFFFF;
    mcontext->gregs[REG_RDX] = (value >> 32) & 0xFFFFFFFF;
    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::emulate_xsetbv(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t reg = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;
    uint64_t value = mcontext->gregs[REG_RAX] & 0xFFFFFFFF;
    value |= ((uint64_t)(mcontext->gregs[REG_RDX] & 0xFFFFFFFF) << 32);
    xsetbv(reg, value);
    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::sigsegv(mcontext_t *mcontext) {
    uintptr_t rip = mcontext->gregs[REG_RIP];
    size_t size = 15;
    uint64_t pc = rip;
    const uint8_t *ptr = (const uint8_t *)pc;
    if (!cs_disasm_iter(mCapstone, &ptr, &size, &pc, mInstruction)) {
        cs_err cserr = cs_errno(mCapstone);
        PV_POSIX_ERROR(EINVAL, "cs_disasm_iter failed: %s (%d)\n", cs_strerror(cserr), cserr);
    }

    SignalLog("%p: %s %s %p\n", (void*)rip, mInstruction->mnemonic, mInstruction->op_str, (void*)mcontext->gregs[REG_CR2]);

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
    case X86_INS_MOV:
        /* This could be either a control register read/write or a page fault we need to emulate */
        emulate_mov(mcontext, mInstruction);
        break;
    case X86_INS_XGETBV:
        emulate_xgetbv(mcontext, mInstruction);
        break;
    case X86_INS_XSETBV:
        emulate_xsetbv(mcontext, mInstruction);
        break;

    default:
        // If the sigsegv is not due to a privileged instruction, its probably a page fault
        emulate_mmu(mcontext, mInstruction);
        break;
    }
}

void pv::Machine::initSingleThread() {
    gMachine = this;

    size_t sigstksz = SIGSTKSZ;
    mSigStack = stack_t {
        .ss_sp = malloc(sigstksz),
        .ss_flags = 0,
        .ss_size = sigstksz,
    };

    if (cs_err err = cs_open(CS_ARCH_X86, CS_MODE_64, &mCapstone)) {
        PV_POSIX_ERROR(err, "cs_open: %s (%d)\n", cs_strerror(err), err);
    }

    cs_option(mCapstone, CS_OPT_DETAIL, CS_OPT_ON);

    mInstruction = cs_malloc(mCapstone);
    if (!mInstruction) {
        PV_POSIX_ERROR(ENOMEM, "cs_malloc: %s\n", strerror(ENOMEM));
    }

    sigset_t sigset;
    PV_POSIX_CHECK(sigprocmask(SIG_BLOCK, nullptr, &sigset));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGSEGV));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGILL));
    PV_POSIX_CHECK(sigprocmask(SIG_SETMASK, &sigset, nullptr));

    PV_POSIX_CHECK(sigaltstack(&mSigStack, nullptr));

    struct sigaction sigill {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            gMachine->sigsegv(mcontext);
            SignalLog("sigill handled: %p\n", (void*)mcontext->gregs[REG_RIP]);
        },
        .sa_flags = SA_SIGINFO | SA_ONSTACK,
    };

    struct sigaction sigsegv {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            gMachine->sigsegv(mcontext);
            SignalLog("sigsegv handled: %p\n", (void*)mcontext->gregs[REG_RIP]);
        },
        .sa_flags = SA_SIGINFO | SA_ONSTACK,
    };

    PV_POSIX_CHECK(sigaction(SIGILL, &sigill, nullptr));
    PV_POSIX_CHECK(sigaction(SIGSEGV, &sigsegv, nullptr));
}

km::VirtualRangeEx pv::Machine::getSharedMemory() {
    return km::VirtualRangeEx::of(gSharedHostMemory, kSharedMemorySize);
}

void pv::Machine::init() {
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);

    PV_POSIX_CHECK((gSharedMemoryFd = memfd_create("pv_host", 0)));
    PV_POSIX_CHECK(ftruncate64(gSharedMemoryFd, kSharedMemorySize));
    PV_POSIX_ASSERT((gSharedHostMemory = mmap(nullptr, kSharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, gSharedMemoryFd, 0)) != MAP_FAILED);
    gSharedHostBase = gSharedHostMemory.address;

    if (int err = rpmalloc_initialize(&gMallocInterface)) {
        throw std::runtime_error(std::format("rpmalloc_initialize: {}", err));
    }
}

void pv::Machine::finalize() {
    rpmalloc_dump_statistics(stderr);
    rpmalloc_finalize();

    PV_POSIX_CHECK(munmap(gSharedHostMemory, kSharedMemorySize));
    PV_POSIX_CHECK(close(gSharedMemoryFd));
    gSharedHostBase = 0;
    gSharedHostMemory = nullptr;
    gSharedMemoryFd = -1;
}

void pv::Machine::initChild() {
    rpmalloc_thread_initialize();
}

void pv::Machine::finalizeChild() {
    rpmalloc_thread_finalize();
}

void *pv::sharedObjectMalloc(size_t size) {
    return rpmalloc(size);
}

void *pv::sharedObjectAlignedAlloc(size_t align, size_t size) {
    return rpaligned_alloc(align, size);
}

void pv::sharedObjectFree(void *ptr) {
    return rpfree(ptr);
}
