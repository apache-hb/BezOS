#include "pvtest/machine.hpp"
#include "pvtest/msr.hpp"
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

pv::Machine::Machine(size_t cores, off64_t memorySize, IModelRegisterSet *msrs)
    : mPageBuilder(48, 48, km::GetDefaultPatLayout())
    , mMemory(memorySize)
    , mMsrSet(msrs)
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
    if (mMsrSet == nullptr) {
        SignalAssert("Unexpected rdmsr without msr set\n");
    }

    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;

    uint64_t value = mMsrSet->rdmsr(msr);
    mcontext->gregs[REG_RAX] = value & 0xFFFFFFFF;
    mcontext->gregs[REG_RDX] = (value >> 32) & 0xFFFFFFFF;
    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::emulate_wrmsr(mcontext_t *mcontext, cs_insn *insn) {
    if (mMsrSet == nullptr) {
        SignalAssert("Unexpected wrmsr without msr set\n");
    }

    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;
    uint32_t low = mcontext->gregs[REG_RAX] & 0xFFFFFFFF;
    uint32_t high = mcontext->gregs[REG_RDX] & 0xFFFFFFFF;

    uint64_t value = low | ((uint64_t)high << 32);
    mMsrSet->wrmsr(msr, value);

    mcontext->gregs[REG_RIP] += insn->size;
}

void pv::Machine::emulate_hlt(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_iretq(mcontext_t *mcontext, cs_insn *insn) { }
void pv::Machine::emulate_mmu(mcontext_t *mcontext, cs_insn *insn) { }

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
